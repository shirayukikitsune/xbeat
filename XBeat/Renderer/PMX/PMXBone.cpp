#include "PMXBone.h"
#include "PMXModel.h"
#include "PMXMaterial.h"
#include <cfloat>
#include <algorithm>

using namespace Renderer::PMX;

void detail::RootBone::Transform(const btVector3& angles, const btVector3& offset, DeformationOrigin origin)
{
	m_transform.AppendRotation(DirectX::XMQuaternionRotationRollPitchYawFromVector(angles.get128()));
	m_transform.AppendTranslation(offset.get128());

	m_touched = true;

	Update();
}

void detail::RootBone::Transform(const btTransform& transform, DeformationOrigin origin)
{
	m_transform *= (DirectX::XMTRANSFORM)transform;

	m_touched = true;

	Update();
}

void detail::RootBone::Rotate(const btVector3& axis, float angle, DeformationOrigin origin)
{
	m_transform.AppendRotation(DirectX::XMQuaternionRotationAxis(axis.get128(), angle));

	m_touched = true;

	Update();
}

void detail::RootBone::Rotate(const btVector3& angles, DeformationOrigin origin)
{
	m_transform.AppendRotation(DirectX::XMQuaternionRotationRollPitchYawFromVector(angles.get128()));

	m_touched = true;

	Update();
}

void detail::RootBone::Translate(const btVector3& offset, DeformationOrigin origin)
{
	m_transform.AppendTranslation(offset.get128());

	m_touched = true;

	Update();
}

void detail::RootBone::ResetTransform()
{
	m_transform = DirectX::XMTRANSFORM();

	m_touched = true;

	Update();
}

void detail::RootBone::Update()
{
	if (!m_touched) return;

	m_touched = false;

	btTransform t = (btTransform)m_transform;
	for (auto body : m_model->m_rigidBodies) {
		btTransform t2 = body->getBody()->getWorldTransform() * (btTransform)body->getAssociatedBone()->GetInverseTransform() * t;
		body->getBody()->setWorldTransform(t2);
	}

	//for (auto joint : m_model->m_joints) {
		//joint->GetConstraint()->
	//}
}

DirectX::XMVECTOR detail::BoneImpl::GetPosition()
{
	return GetTransform().GetTranslation();
}

void detail::BoneImpl::Initialize()
{
	m_inheritRotation = m_userRotation = m_morphRotation = DirectX::XMQuaternionIdentity();
	m_inheritTranslation = m_userTranslation = m_morphTranslation = DirectX::XMVectorZero();

	m_transform.SetTranslation(startPosition.get128());
	m_transform.SetOffset(startPosition.get128());
	m_inverse.SetTranslation((-startPosition).get128());

	using namespace DirectX;
	if (!HasAnyFlag(BoneFlags::LocalAxis)) {
		auto parent = dynamic_cast<detail::BoneImpl*>(GetParent());
		if (parent) m_localAxis = parent->GetLocalAxis();

		m_localAxis = XMMatrixIdentity();
	} 
	else {
		m_localAxis.r[0] = XMVectorSetW(localAxes.xDirection.get128(), 0.0f);
		m_localAxis.r[2] = localAxes.zDirection.get128();
		m_localAxis.r[1] = XMVectorSetW(XMVector3Cross(m_localAxis.r[0], m_localAxis.r[2]), 0.0f);
		m_localAxis.r[2] = XMVectorSetW(XMVector3Cross(m_localAxis.r[0], m_localAxis.r[1]), 0.0f);
		m_localAxis.r[3] = XMVectorSetW(axisTranslation.get128(), 1.0f);
		m_localAxis = XMMatrixTranspose(m_localAxis);
	}
}

void detail::BoneImpl::InitializeDebug(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	if (!HasAnyFlag(BoneFlags::IK) && HasAnyFlag(BoneFlags::View))
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

DirectX::XMVECTOR XM_CALLCONV detail::BoneImpl::GetOffsetPosition()
{
	detail::BoneImpl* parent = dynamic_cast<detail::BoneImpl*>(GetParent());
	if (!parent) return startPosition.get128();
	
	return startPosition.get128() - parent->startPosition.get128();
}

DirectX::XMVECTOR XM_CALLCONV detail::BoneImpl::GetEndPosition()
{
	DirectX::XMVECTOR p;

	if (HasAnyFlag(BoneFlags::Attached)) {
		auto other = m_model->GetBoneById(this->size.attachTo);
		p = other->GetPosition();
	}
	else {
		p = GetTransform().PointTransform(DirectX::XMVectorSet(this->size.length[0], this->size.length[1], this->size.length[2], 1.0f));
	}

	return p;
}

DirectX::XMVECTOR XM_CALLCONV detail::BoneImpl::GetStartPosition()
{
	return startPosition.get128();
}

DirectX::XMTRANSFORM detail::BoneImpl::GetTransform()
{
	return m_transform * GetRootBone()->GetTransform();
}

void detail::BoneImpl::Update()
{
	//if (!m_dirty) return;

	// Work on translation
	DirectX::XMVECTOR translation = DirectX::XMVectorZero();
	if (HasAnyFlag(BoneFlags::LocalInheritance))
		translation = GetParent()->GetLocalTransform().GetTranslation();
	else {
		DirectX::XMVECTOR tmp = DirectX::XMVectorZero();
		if (HasAnyFlag(BoneFlags::InheritTranslation)) {
			auto target = dynamic_cast<detail::BoneImpl*>(m_model->GetBoneById(this->inherit.from));
			tmp = target->m_inheritTranslation * this->inherit.rate;
		}
		translation = m_userTranslation + m_morphTranslation + tmp;
	}

	// Work on rotation..
	DirectX::XMVECTOR rotation = DirectX::XMQuaternionIdentity();
	if (HasAnyFlag(BoneFlags::LocalInheritance))
		rotation = GetParent()->GetLocalTransform().GetRotationQuaternion();
	else if (HasAnyFlag(BoneFlags::InheritRotation))
		rotation = DirectX::XMQuaternionMultiply(DirectX::XMQuaternionMultiply(m_userRotation, m_morphRotation), DirectX::XMQuaternionSlerp(DirectX::XMQuaternionIdentity(), dynamic_cast<detail::BoneImpl*>(m_model->GetBoneById(this->inherit.from))->m_inheritRotation, this->inherit.rate));
	else
		rotation = DirectX::XMQuaternionMultiply(m_userRotation, m_morphRotation);
	rotation = DirectX::XMQuaternionNormalize(rotation);

	m_inheritRotation = rotation;
	m_inheritTranslation = translation;

	//translation = DirectX::XMVector3Transform(translation, GetLocalAxis()) + GetOffsetPosition();
	//rotation = DirectX::XMQuaternionNormalize(DirectX::XMQuaternionMultiply(DirectX::XMQuaternionConjugate(DirectX::XMQuaternionRotationMatrix(GetLocalAxis())), rotation));

	m_transform = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSplatOne(), GetStartPosition(), rotation, translation);
	if (auto parent = GetParent()) m_transform = parent->GetLocalTransform() * m_transform;

	m_dirty = false;
}

void detail::BoneImpl::Transform(const btVector3& angles, const btVector3& offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable)))
		return;

	m_dirty = true;

	auto local = DirectX::XMQuaternionRotationRollPitchYawFromVector(angles.get128());

	m_userRotation = DirectX::XMQuaternionMultiply(local, m_userRotation);
	m_userTranslation = m_userTranslation + offset.get128();
}

void detail::BoneImpl::Transform(const btTransform& transform, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable)))
		return;

	m_dirty = true;

	m_userRotation = DirectX::XMQuaternionMultiply(transform.getRotation().get128(), m_userRotation);
	m_userTranslation = m_userTranslation + transform.getOrigin().get128();
}

void detail::BoneImpl::Rotate(const btVector3& axis, float angle, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable)))
		return;

	m_dirty = true;

	auto local = DirectX::XMQuaternionRotationAxis(axis.get128(), angle);

	m_userRotation = DirectX::XMQuaternionMultiply(local, m_userRotation);
}

void detail::BoneImpl::Rotate(const btVector3& angles, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable)))
		return;

	m_dirty = true;

	auto local = DirectX::XMQuaternionRotationRollPitchYawFromVector(angles.get128());

	m_userRotation = DirectX::XMQuaternionMultiply(local, m_userRotation);
}

void detail::BoneImpl::Translate(const btVector3& offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable)))
		return;

	m_dirty = true;

	m_userTranslation = m_userTranslation + offset.get128();
}

void detail::BoneImpl::ResetTransform()
{
	m_dirty = true;
	m_userTranslation = DirectX::XMVectorZero();
	m_userRotation = DirectX::XMQuaternionIdentity();
}

void detail::BoneImpl::ApplyPhysicsTransform(DirectX::XMTRANSFORM &transform)
{
	//m_transform = transform;
	//m_transform.SetOffset(GetOffsetPosition());
	//m_dirty = true;
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

	m_dirty = true;

	// Now calculate the transformations
	DirectX::XMVECTOR position = DirectX::XMVectorZero(), rotation = DirectX::XMQuaternionIdentity(), tmp;
	for (auto &pair : appliedMorphs) {
		for (auto &data : pair.first->data) {
			if (data.bone.index == this->GetId()) {
				tmp = pair.second * DirectX::XMVectorSet(data.bone.movement[0], data.bone.movement[1], data.bone.movement[2], 0.0f);
				position = position + tmp;
				rotation = DirectX::XMQuaternionMultiply(rotation, DirectX::XMQuaternionSlerp(DirectX::XMQuaternionIdentity(), DirectX::XMVectorSet(data.bone.rotation[0], data.bone.rotation[1], data.bone.rotation[2], data.bone.rotation[3]), pair.second));
				rotation = DirectX::XMQuaternionNormalize(rotation);
			}
		}
	}
	m_morphRotation = rotation;
	m_morphTranslation = position;
}

bool XM_CALLCONV detail::BoneImpl::Render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	DirectX::XMMATRIX w;

	if (m_primitive)
	{
		float lensq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(GetEndPosition() - GetPosition()));
		if (lensq == 0) {
			return true;
		}

		w = DirectX::XMMatrixScaling(0.3f, sqrtf(lensq), 0.3f) * m_transform;

		m_primitive->Draw(w, view, projection);
	}

	return true;
}

Bone* detail::BoneImpl::GetRootBone() { 
	return m_model->GetRootBone();
}