#include "PMXBone.h"

#include "PMXMaterial.h"
#include "PMXModel.h"

#include <algorithm>
#include <cfloat>

using namespace PMX;

void Bone::UpdateChildren() {
	for (auto &Child : m_children) {
		Child->Update();
		Child->UpdateChildren();
	}
}

void detail::RootBone::Transform(btVector3 angles, btVector3 offset, DeformationOrigin origin)
{
	btQuaternion q;
	q.setEulerZYX(angles.x(), angles.y(), angles.z());
	m_transform.setRotation(q * m_transform.getRotation());
	m_transform.setOrigin(m_transform.getOrigin() + offset);

	m_inverse = m_transform.inverse();
}

void detail::RootBone::Transform(const btTransform& transform, DeformationOrigin origin)
{
	m_transform *= transform;

	m_inverse = m_transform.inverse();
}

void detail::RootBone::Rotate(btVector3 axis, float angle, DeformationOrigin origin)
{
	btQuaternion q;
	q.setRotation(axis, angle);
	m_transform.setRotation(q * m_transform.getRotation());

	m_inverse = m_transform.inverse();
}

void detail::RootBone::Rotate(btVector3 angles, DeformationOrigin origin)
{
	btQuaternion q;
	q.setEulerZYX(angles.x(), angles.y(), angles.z());
	m_transform.setRotation(q * m_transform.getRotation());

	m_inverse = m_transform.inverse();
}

void detail::RootBone::Translate(btVector3 offset, DeformationOrigin origin)
{
	m_transform.setOrigin(m_transform.getOrigin() + offset);

	m_inverse = m_transform.inverse();
}

void detail::RootBone::ResetTransform()
{
	m_transform.setIdentity();
	m_inverse.setIdentity();
}

void detail::BoneImpl::Initialize()
{
	m_inheritRotation = m_userRotation = m_morphRotation = m_ikRotation = btQuaternion::getIdentity();
	m_inheritTranslation = m_userTranslation = m_morphTranslation = btVector3(0, 0, 0);

	btVector3 up, axis, direction;
	up = btVector3(0, 1, 0);
	direction = GetEndPosition() - GetPosition();
	axis = up.cross(direction);
	if (axis.length2() > 0) {
		// cosO = a.b/||a||*||b||
		m_debugRotation.setRotation(axis, acosf(up.dot(direction) / direction.length()));
	}
	else m_debugRotation = btQuaternion::getIdentity();

	m_inverse.setOrigin(-startPosition);
	m_transform.setOrigin(startPosition);

	m_parent = m_model->GetBoneById(m_parentId);
	m_parent->m_children.push_back(this);

	using namespace DirectX;
	if (!HasAnyFlag((uint16_t)BoneFlags::LocalAxis)) {
		m_localAxis = XMMatrixIdentity();

		auto parent = dynamic_cast<detail::BoneImpl*>(GetParent());
		if (parent) m_localAxis = parent->GetLocalAxis();
	}
	else {
		m_localAxis.r[0] = XMVectorSetW(localAxes.xDirection, 0.0f);
		m_localAxis.r[2] = localAxes.zDirection;
		m_localAxis.r[1] = XMVectorSetW(XMVector3Cross(m_localAxis.r[0], m_localAxis.r[2]), 0.0f);
		m_localAxis.r[2] = XMVectorSetW(XMVector3Cross(m_localAxis.r[0], m_localAxis.r[1]), 0.0f);
		m_localAxis.r[3] = XMVectorSetW(axisTranslation, 1.0f);
		m_localAxis = XMMatrixTranspose(m_localAxis);
	}

	if (ikData) {
		ikData->targetBone = static_cast<detail::BoneImpl*>(m_model->GetBoneById(ikData->targetIndex));
		ikData->chainLength = 0.0f;

		for (auto &Link : ikData->links) {
			Link.bone = static_cast<detail::BoneImpl*>(m_model->GetBoneById(Link.boneIndex));
			// Hack to fix models imported from PMD
			if (Link.bone->name.japanese.find(L"ひざ") != std::wstring::npos && Link.limitAngle == false) {
				Link.limitAngle = true;
				Link.limits.lower[0] = -DirectX::XM_PI;
				Link.limits.lower[1] = Link.limits.lower[2] = Link.limits.upper[0] = Link.limits.upper[1] = Link.limits.upper[2] = 0.0f;
			}
			ikData->chainLength += Link.bone->getLength();
		}
	}
}

void detail::BoneImpl::InitializeDebug(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	/*if (!HasAnyFlag((uint16_t)BoneFlags::IK) && HasAnyFlag((uint16_t)BoneFlags::View))
		m_primitive = DirectX::GeometricPrimitive::CreateCylinder(d3d->GetDeviceContext());
	else */if (HasAnyFlag((uint16_t)BoneFlags::IK))
		m_primitive = DirectX::GeometricPrimitive::CreateSphere(d3d->GetDeviceContext(), 2.5f);
}

void detail::BoneImpl::Terminate()
{
	m_primitive.reset();
}

DirectX::XMMATRIX XM_CALLCONV detail::BoneImpl::GetLocalAxis()
{
	return m_localAxis;
}

btVector3 detail::BoneImpl::GetOffsetPosition()
{
	detail::BoneImpl* parent = dynamic_cast<detail::BoneImpl*>(GetParent());
	if (!parent) return startPosition;
	
	return startPosition - parent->startPosition;
}

btVector3 detail::BoneImpl::GetEndPosition()
{
	btVector3 p;

	if (HasAnyFlag((uint16_t)BoneFlags::Attached)) {
		auto other = m_model->GetBoneById(this->size.attachTo);
		p = other->GetPosition();
	}
	else {
		p = btVector3(this->size.length[0], this->size.length[1], this->size.length[2]);
		p = GetTransform() * p;
	}

	return p;
}

float detail::BoneImpl::getLength()
{
	if (!HasAnyFlag((uint16_t)BoneFlags::Attached)) {
		return sqrtf(size.length[0] * size.length[0] + size.length[1] * size.length[1] + size.length[2] * size.length[2]);
	}

	auto other = static_cast<detail::BoneImpl*>(m_model->GetBoneById(this->size.attachTo));
	return other->startPosition.distance(startPosition);
}

btVector3 detail::BoneImpl::GetPosition()
{
	detail::BoneImpl* parent = dynamic_cast<detail::BoneImpl*>(GetParent());
	if (!parent) return GetLocalTransform() * startPosition;

	return parent->m_transform * GetOffsetPosition();
}

btVector3 detail::BoneImpl::GetStartPosition()
{
	return startPosition;
}

void detail::BoneImpl::Update()
{
	btVector3 translation(0, 0, 0);
	btQuaternion rotation = btQuaternion::getIdentity();

	auto parent = GetParent();
	if (HasAnyFlag((uint16_t)BoneFlags::LocallyAttached)) {
		translation = parent->GetLocalTransform().getOrigin();
	}
	else if (HasAnyFlag((uint16_t)BoneFlags::TranslationAttached)) {
		auto target = dynamic_cast<BoneImpl*>(m_model->GetBoneById(this->inherit.from));
		if (target) translation = target->m_inheritTranslation;
	}
	translation *= inherit.rate;

	if (HasAnyFlag((uint16_t)BoneFlags::LocallyAttached)) {
		rotation = parent->GetLocalTransform().getRotation();
	}
	else {
		if (HasAnyFlag((uint16_t)BoneFlags::RotationAttached)) {
			auto target = dynamic_cast<BoneImpl*>(m_model->GetBoneById(this->inherit.from));
			if (target) {
				rotation = target->m_inheritRotation;
			}
		}
	}
	rotation = btQuaternion::getIdentity().slerp(rotation, inherit.rate);

	m_inheritRotation = rotation;
	m_inheritTranslation = translation;

	translation = (translation + m_userTranslation + m_morphTranslation + GetOffsetPosition());

	rotation = m_ikRotation * m_morphRotation * m_userRotation * rotation;
	rotation.normalize();

	m_transform = HasAnyFlag((uint16_t)BoneFlags::LocallyAttached) ? btTransform(rotation, translation) : parent->GetTransform() * btTransform(rotation, translation);
}

void detail::BoneImpl::Transform(btVector3 angles, btVector3 offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable))
		return;

	btQuaternion local;
	local.setEulerZYX(angles.x(), angles.y(), angles.z());

	m_userRotation *= local;
	m_userTranslation += offset;
}

void detail::BoneImpl::Transform(const btTransform& transform, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable))
		return;

	m_userRotation *= transform.getRotation();
	m_userTranslation += transform.getOrigin();
}

void detail::BoneImpl::Rotate(btVector3 axis, float angle, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable))
		return;

	btQuaternion local;
	local.setRotation(axis, angle);

	m_userRotation *= local;
}

void detail::BoneImpl::Rotate(btVector3 angles, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable))
		return;

	btQuaternion local;
	local.setEulerZYX(angles.x(), angles.y(), angles.z());

	m_userRotation *= local;
}

void detail::BoneImpl::Translate(btVector3 offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable))
		return;

	m_userTranslation += offset;
}

void detail::BoneImpl::ResetTransform()
{
	m_userTranslation.setZero();
	m_userRotation = btQuaternion::getIdentity();
}

void detail::BoneImpl::ApplyPhysicsTransform(btTransform &transform)
{
	m_transform = transform;
}

void detail::BoneImpl::ApplyMorph(Morph *morph, float weight)
{
	auto i = ([this, morph](){ for (auto i = appliedMorphs.begin(); i != appliedMorphs.end(); ++i) { if (i->first == morph) return i; } return appliedMorphs.end(); })();
	if (weight == 0.0f) {
		if (i != appliedMorphs.end())
			appliedMorphs.erase(i);
		else return;
	}
	else {
		if (i == appliedMorphs.end())
			appliedMorphs.push_back(std::make_pair(morph, weight));
		else
			i->second = weight;
	}

	// Now calculate the transformations
	DirectX::XMVECTOR position = DirectX::XMVectorZero(), rotation = DirectX::XMQuaternionIdentity();
	for (auto &pair : appliedMorphs) {
		for (auto &data : pair.first->data) {
			if (data.bone.index == this->GetId()) {
				position = position + DirectX::XMVectorScale(DirectX::XMVectorSet(data.bone.movement[0], data.bone.movement[1], data.bone.movement[2], 0.0f), pair.second);
				rotation = DirectX::XMQuaternionMultiply(rotation, DirectX::XMQuaternionSlerp(DirectX::XMQuaternionIdentity(), DirectX::XMVectorSet(data.bone.rotation[0], data.bone.rotation[1], data.bone.rotation[2], data.bone.rotation[3]), pair.second));
				rotation = DirectX::XMQuaternionNormalize(rotation);
			}
		}
	}
	m_morphRotation.set128(rotation);
	m_morphTranslation.set128(position);
}

bool XM_CALLCONV detail::BoneImpl::Render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	DirectX::XMMATRIX w;

	if (m_primitive)
	{
		w = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(0.3f, 0.3f, 0.3f, 1), DirectX::XMVectorZero(), getRotation().get128(), GetPosition().get128());

		m_primitive->Draw(w, view, projection, DirectX::Colors::Red);
	}

	return true;
}

Bone* detail::BoneImpl::GetRootBone() { 
	return m_model->GetRootBone();
}

void detail::BoneImpl::PerformIK() {
	if (!ikData || ikData->links.empty() || ikData->links[0].bone->Simulated || ikData->chainLength <= 0.00001f)
		return;

	btVector3 TargetPosition = this->GetPosition();
	btVector3 RootPosition = ikData->links.back().bone->GetPosition();

	// Determine if the target position is reachable
	if (RootPosition.distance(TargetPosition) > ikData->chainLength) {
		// If unreachable, move the target point to a reachable point colinear to the root bone position and the original target point
		TargetPosition = (TargetPosition - RootPosition).normalized() * ikData->chainLength + RootPosition;
		assert(RootPosition.distance(TargetPosition) <= ikData->chainLength + 0.0001f);
	}

	btVector3 EndPosition = ikData->targetBone->GetPosition();
	// Check if the last joint position is close to the target point
	if (EndPosition.distance(TargetPosition) < 0.001f) {
		return;
	}

	btQuaternion OldRotation = ikData->targetBone->getRotation();

	for (int Iteration = 0; Iteration < ikData->loopCount; ++Iteration) {
		btVector3 EndPosition = ikData->targetBone->GetPosition();
		// Check if the last joint position is close to the target point
		if (EndPosition.distance(TargetPosition) < 0.001f) {
			return;
		}

		// Here we are assuming that all bones of the chain are interconnected
		std::vector<btVector3> TargetPositions;
		// The first position of the vector is the root of the chain, and the last, the target point
		for (int Index = ikData->links.size() - 1; Index >= 0; --Index) {
			TargetPositions.emplace_back(ikData->links[Index].bone->GetPosition());
		}
		
		TargetPositions.emplace_back(TargetPosition);

		auto performInnerIteraction = [](btVector3 &ParentBonePosition, btVector3 &P1, btVector3 &P2, IK::Node &Link, bool Reverse) {
			float Distance = P1.distance(P2);
			if (Distance <= 0.00001f) {
				P1 = P2;
				return;
			}
			float Ratio = Link.bone->getLength() / Distance;

			P1 = P2.lerp(P1, Ratio);

			// Now apply rotation constraints if needed
			if (!Link.limitAngle || ParentBonePosition.isZero())
				return;

			btVector3 Direction = (P1 - P2).normalize();
			btVector3 BoneDirection = (Link.bone->GetEndPosition() - Link.bone->GetPosition()).normalize();
			if (!Reverse) BoneDirection = -BoneDirection;

			float Angle = btAcos(Direction.dot(BoneDirection));
			if (Angle < 0.0001f) return;
			btVector3 Axis = BoneDirection.cross(Direction);
			if (Axis.length2() < 0.0000001f) return;
			Axis.normalize();

			btQuaternion Rotation(Axis, Angle);
			btMatrix3x3 Auxiliar;
			Auxiliar.setRotation(Rotation);
			float x, y, z;
			Auxiliar.getEulerZYX(z, y, x);

			btClamp(x, Link.limits.lower[0], Link.limits.upper[0]);
			btClamp(y, Link.limits.lower[1], Link.limits.upper[1]);
			btClamp(z, Link.limits.lower[2], Link.limits.upper[2]);

			Rotation.setEulerZYX(z, y, x);

			P1 = quatRotate(Rotation, BoneDirection) * Link.bone->getLength() + P2;
		};

		btVector3 P0;
		// Stage 1: Forward reaching
		for (int Index = TargetPositions.size() - 2; Index >= 0; --Index) {
			auto& Link = ikData->links[ikData->links.size() - Index - 1];
			performInnerIteraction(P0, TargetPositions[Index], TargetPositions[Index + 1], Link, false);
			P0 = TargetPositions[Index + 1];
		}

		// Set the root position its initial position
		TargetPositions[0] = RootPosition;

		// Stage 2: Backward reaching
		P0.setZero();
		for (int Index = 0; Index < TargetPositions.size() - 1; ++Index) {
			auto& Link = ikData->links[ikData->links.size() - Index - 1];
			performInnerIteraction(P0, TargetPositions[Index + 1], TargetPositions[Index], Link, true);
			P0 = TargetPositions[Index];
		}

		// Apply the new positions as rotations
		for (int Index = 0; Index < TargetPositions.size() - 1; ++Index) {
			auto& Link = ikData->links[ikData->links.size() - Index - 1];
			btVector3 position = Link.bone->GetPosition();

			btVector3 CurrentDirection = (Link.bone->GetEndPosition() - position).normalized();
			btVector3 DesiredDirection = (TargetPositions[Index + 1] - position).normalized();

			btVector3 Axis = CurrentDirection.cross(DesiredDirection);
			float Dot = CurrentDirection.dot(DesiredDirection);
			// No need to update if position is too close
			if (Axis.length2() < 0.000001f || Dot > 0.999999f) continue;
			
			Axis.normalize();
			float Angle = btAcos(Dot);

			Link.bone->m_ikRotation *= btQuaternion(Axis, Angle);

			Link.bone->Update();

			position = Link.bone->GetPosition();
		}
		ikData->targetBone->Update();
	}
}
