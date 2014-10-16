#include "PMXBone.h"
#include "PMXModel.h"
#include "PMXMaterial.h"
#include <cfloat>
#include <algorithm>

using namespace PMX;

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
	m_physicsTransform.setIdentity();

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

		for (auto &Link : ikData->links) {
			Link.bone = static_cast<detail::BoneImpl*>(m_model->GetBoneById(Link.boneIndex));
			// Hack to fix models imported from PMD
			if (Link.bone->name.japanese.find(L"ひざ") != std::wstring::npos) {
				Link.limitAngle = true;
				Link.limits.lowerLimit.setEulerZYX(0, 0, -DirectX::XM_PI);
				Link.limits.upperLimit = btQuaternion::getIdentity();
			}
			else if (Link.limitAngle) {
				Link.limits.lowerLimit.setEulerZYX(Link.limits.lower[2], Link.limits.lower[1], Link.limits.lower[0]);
				Link.limits.upperLimit.setEulerZYX(Link.limits.upper[2], Link.limits.upper[1], Link.limits.upper[0]);
			}
		}
	}
}

void detail::BoneImpl::InitializeDebug(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	if (!HasAnyFlag((uint16_t)BoneFlags::IK) && HasAnyFlag((uint16_t)BoneFlags::View))
		m_primitive = DirectX::GeometricPrimitive::CreateCylinder(d3d->GetDeviceContext());
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
		p = GetTransform()(btVector3(this->size.length[0], this->size.length[1], this->size.length[2]));
	}

	return p;
}

btVector3 detail::BoneImpl::GetPosition()
{
	return GetTransform() * startPosition;
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
		translation += parent->GetLocalTransform().getOrigin();
	}
	else if (HasAnyFlag((uint16_t)BoneFlags::TranslationAttached)) {
		auto target = dynamic_cast<BoneImpl*>(m_model->GetBoneById(this->inherit.from));
		if (target) translation += target->m_inheritTranslation;
	}
	translation *= inherit.rate;

	if (HasAnyFlag((uint16_t)BoneFlags::LocallyAttached)) {
		rotation *= parent->GetLocalTransform().getRotation();
	}
	else if (HasAnyFlag((uint16_t)BoneFlags::RotationAttached)) {
		auto target = dynamic_cast<BoneImpl*>(m_model->GetBoneById(this->inherit.from));
		if (target) {
			rotation = rotation * target->m_inheritRotation * target->m_ikRotation;
		}
	}
	rotation = btQuaternion::getIdentity().slerp(rotation, inherit.rate);

	m_inheritRotation = rotation;
	m_inheritTranslation = translation;

	translation = (translation + m_userTranslation + m_morphTranslation + GetOffsetPosition());

	rotation = rotation * m_userRotation * m_morphRotation * m_ikRotation;

	m_transform = HasAnyFlag((uint16_t)BoneFlags::LocallyAttached) ? btTransform(rotation, translation) : parent->GetTransform() * btTransform(rotation, translation);

	m_dirty = false;
}

void detail::BoneImpl::Transform(btVector3 angles, btVector3 offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable))
		return;

	setDirty();

	btQuaternion local;
	local.setEulerZYX(angles.x(), angles.y(), angles.z());

	m_userRotation *= local;
	m_userTranslation += offset;
}

void detail::BoneImpl::Transform(const btTransform& transform, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable))
		return;

	setDirty();

	m_userRotation *= transform.getRotation();
	m_userTranslation += transform.getOrigin();
}

void detail::BoneImpl::Rotate(btVector3 axis, float angle, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable))
		return;

	setDirty();

	btQuaternion local;
	local.setRotation(axis, angle);

	m_userRotation *= local;
}

void detail::BoneImpl::Rotate(btVector3 angles, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable))
		return;

	setDirty();

	btQuaternion local;
	local.setEulerZYX(angles.x(), angles.y(), angles.z());

	m_userRotation *= local;
}

void detail::BoneImpl::Translate(btVector3 offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable))
		return;

	setDirty();

	m_userTranslation += offset;
}

void detail::BoneImpl::ResetTransform()
{
	setDirty();
	m_userTranslation.setZero();
	m_userRotation = btQuaternion::getIdentity();
}

void detail::BoneImpl::ApplyPhysicsTransform(btTransform &transform)
{
	m_physicsTransform = m_inverse * transform;
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

	setDirty();

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
		float lensq = (GetEndPosition() - GetPosition()).length2();
		if (lensq == 0) {
			return true;
		}

		btTransform t(m_debugRotation, startPosition);
		t *= m_transform;

		w = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(0.3f, sqrtf(lensq), 0.3f, 1), DirectX::XMVectorZero(), t.getRotation().get128(), t.getOrigin().get128());

		m_primitive->Draw(w, view, projection);
	}

	return true;
}

Bone* detail::BoneImpl::GetRootBone() { 
	return m_model->GetRootBone();
}

void detail::BoneImpl::setDirty() {
	m_dirty = true;

	for (auto child : m_children) {
		static_cast<BoneImpl*>(child)->setDirty();
	}
}

void detail::BoneImpl::PerformIK() {
	btVector3 Destination = this->GetPosition();
	auto InitialRotation = ikData->targetBone->GetTransform().getRotation();

	for (int Iteration = 0; Iteration < ikData->loopCount; ++Iteration) {
		for (int Index = 0; Index < ikData->links.size(); ++Index) {
			auto &Link = ikData->links[Index];
			auto Bone = static_cast<detail::BoneImpl*>(Link.bone);
			btVector3 CurrentPosition = Bone->GetPosition();
			btVector3 TargetPosition = ikData->targetBone->GetPosition();

			if (CurrentPosition == Destination || CurrentPosition == TargetPosition) continue;

			btTransform TransformInverse = Link.bone->GetTransform().inverse();
			btVector3 LocalDestination = TransformInverse(Destination);
			btVector3 LocalTarget = TransformInverse(TargetPosition);

			if (LocalDestination.distance2(LocalTarget) <= 0.0001f) {
				Iteration = ikData->loopCount;
				break;
			}

			LocalDestination.normalize();
			LocalTarget.normalize();

			auto Dot = LocalDestination.dot(LocalTarget);
			if (Dot > 1.0f) continue;

			auto Angle = acosf(Dot);
			if (fabsf(Angle) < 0.00000001f) continue;

			Angle = std::min(std::max(Angle, -ikData->angleLimit * DirectX::XM_PI), ikData->angleLimit * DirectX::XM_PI);

			btVector3 Axis = LocalTarget.cross(LocalDestination);
			if (Axis.length2() < 0.00000001f && Iteration != 0) continue;

			Axis.normalize();

			btQuaternion Rotation(Axis, Angle);
			// clamp rotation values
			if (Link.limitAngle) {
#if 0
				if (Iteration == 0) {
					if (Angle < 0.0f)
						Angle = -Angle;
					Rotation.setRotation(btVector3(1.0f, 0.0f, 0.0f), Angle);
				}
				else {
					btMatrix3x3 Matrix;
					float x, y, z;
					Matrix.setRotation(Rotation);
					Matrix.getEulerZYX(z, y, x);

					if (Axis.z() < 0.0f) {
						x = std::min(x, Link.limits.upper[0]);
						y = std::min(y, Link.limits.upper[1]);
						z = std::min(z, Link.limits.upper[2]);
					}
					else {
						x = std::max(x, Link.limits.lower[0]);
						y = std::max(y, Link.limits.lower[1]);
						z = std::max(z, Link.limits.lower[2]);
					}

					Rotation.setEulerZYX(z, y, x);
				}
#else
				Rotation.setMin(Link.limits.upperLimit);
				Rotation.setMax(Link.limits.lowerLimit);
				Rotation.normalize();
#endif
			}
			Bone->m_ikRotation = (Iteration == 0 ? Rotation : Bone->m_ikRotation * Rotation);
			Bone->Update();

			for (int UpdateIndex = Index; UpdateIndex >= 0; --UpdateIndex) {
				ikData->links[UpdateIndex].bone->Update();
			}
			ikData->targetBone->Update();
		}
	}
}
