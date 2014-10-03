#include "PMXBone.h"
#include "PMXModel.h"
#include "PMXMaterial.h"
#include <cfloat>
#include <algorithm>

using namespace PMX;

void detail::RootBone::Transform(DirectX::FXMVECTOR angles, DirectX::FXMVECTOR offset, DeformationOrigin origin)
{
	m_transform.AppendRotation(DirectX::XMQuaternionRotationRollPitchYawFromVector(angles));
	m_transform.AppendTranslation(offset);

	m_touched = true;

	Update();
}

void detail::RootBone::Transform(const DirectX::XMTRANSFORM& transform, DeformationOrigin origin)
{
	m_transform *= transform;

	m_touched = true;

	Update();
}

void detail::RootBone::Rotate(DirectX::FXMVECTOR axis, float angle, DeformationOrigin origin)
{
	m_transform.AppendRotation(DirectX::XMQuaternionRotationAxis(axis, angle));

	m_touched = true;

	Update();
}

void detail::RootBone::Rotate(DirectX::FXMVECTOR angles, DeformationOrigin origin)
{
	m_transform.AppendRotation(DirectX::XMQuaternionRotationRollPitchYawFromVector(angles));

	m_touched = true;

	Update();
}

void detail::RootBone::Translate(DirectX::FXMVECTOR offset, DeformationOrigin origin)
{
	m_transform.AppendTranslation(offset);

	m_touched = true;

	Update();
}

void detail::RootBone::ResetTransform()
{
	m_transform.Reset();

	m_touched = true;

	Update();
}

void detail::RootBone::Update()
{
	if (!m_touched) return;

	m_touched = false;

	/*btTransform t = (btTransform)m_transform;
	for (auto body : m_model->m_rigidBodies) {
		if (body->getAssociatedBone() != nullptr) {
			btTransform t2 = t * body->getBody()->getWorldTransform();
			body->getBody()->setWorldTransform(t2);
		}
	}*/
}

void detail::BoneImpl::Initialize()
{
	m_inheritRotation = m_userRotation = m_morphRotation = DirectX::XMQuaternionIdentity();
	m_inheritTranslation = m_userTranslation = m_morphTranslation = DirectX::XMVectorZero();

	//m_transform.SetTranslation(startPosition);
	m_inverse.SetTranslation(DirectX::XMVectorNegate(startPosition));

	DirectX::XMVECTOR up, axis, direction;
	up = DirectX::XMVectorSet(0, 1, 0, 0);
	direction = GetEndPosition() - GetPosition();
	axis = DirectX::XMVector3Cross(direction, up);
	if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(axis)) > 0) {
		// cosO = a.b/||a||*||b||
		m_debugRotation = DirectX::XMQuaternionRotationAxis(axis, DirectX::XMScalarACos(DirectX::XMVectorGetX(DirectX::XMVector3Dot(up, direction)) / DirectX::XMVectorGetX(DirectX::XMVector3Length(direction))));
	}
	else m_debugRotation = DirectX::XMQuaternionIdentity();
	
	m_parent = m_model->GetBoneById(m_parentId);
	m_parent->m_children.push_back(this);

	using namespace DirectX;
	if (!HasAnyFlag(BoneFlags::LocalAxis)) {
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
	if (!parent) return startPosition;
	
	return startPosition - parent->startPosition;
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

DirectX::XMVECTOR detail::BoneImpl::GetPosition()
{
	return GetTransform().GetTranslation();
}

DirectX::XMVECTOR XM_CALLCONV detail::BoneImpl::GetStartPosition()
{
	return startPosition;
}

DirectX::XMTRANSFORM detail::BoneImpl::GetTransform()
{
	return m_transform;
}

DirectX::XMTRANSFORM detail::BoneImpl::GetLocalTransform()
{
	return m_transform * m_inverse;
}

void detail::BoneImpl::Update()
{
	DirectX::XMVECTOR translation = DirectX::XMVectorZero();
	DirectX::XMVECTOR rotation = DirectX::XMQuaternionIdentity();

	if (m_parentId != -1) {
		BoneImpl *parent = static_cast<detail::BoneImpl*>(GetParent());

		// Work on translation
#if 1
		if (HasAnyFlag(BoneFlags::LocallyAttached)) {
			//translation = parent->GetLocalTransform().GetTranslation();
		}
		else if (HasAnyFlag(BoneFlags::TranslationAttached)) {
			auto target = static_cast<BoneImpl*>(m_model->GetBoneById(this->inherit.from));
			translation = target->m_inheritTranslation;
		}
		else {
			//translation = parent->m_userTranslation + parent->m_morphTranslation;
		}
		translation = DirectX::XMVectorScale(translation, this->inherit.rate);
#endif

		// Work on rotation..
		if (HasAnyFlag(BoneFlags::LocallyAttached)) {
			//rotation = parent->GetLocalTransform().GetRotationQuaternion();
		}
		else if (HasAnyFlag(BoneFlags::RotationAttached)) {
			auto target = static_cast<BoneImpl*>(m_model->GetBoneById(this->inherit.from));
			rotation = target->m_inheritRotation;
		}
		//else
			//rotation = DirectX::XMQuaternionNormalize(DirectX::XMQuaternionMultiply(parent->m_userRotation, parent->m_morphRotation));
		rotation = DirectX::XMQuaternionSlerp(DirectX::XMQuaternionIdentity(), rotation, this->inherit.rate);
	}

	m_inheritRotation = rotation;
	m_inheritTranslation = translation;

	translation = translation + m_userTranslation + m_morphTranslation;
	rotation = DirectX::XMQuaternionMultiply(rotation, m_userRotation);
	rotation = DirectX::XMQuaternionMultiply(rotation, m_morphRotation);

	//translation = DirectX::XMVector3Transform(translation, GetLocalAxis());
	//rotation = DirectX::XMQuaternionNormalize(DirectX::XMQuaternionMultiply(DirectX::XMQuaternionRotationMatrix(GetLocalAxis()), rotation));

	m_transform = DirectX::XMTRANSFORM(rotation, translation);
	//m_transform = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSplatOne(), translation + startPosition, rotation, translation);
	if (auto parent = GetParent()) m_transform.AppendTransform(parent->GetTransform());

	m_dirty = false;
}

void detail::BoneImpl::Transform(DirectX::FXMVECTOR angles, DirectX::FXMVECTOR offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable)))
		return;

	setDirty();

	auto local = DirectX::XMQuaternionRotationRollPitchYawFromVector(angles);

	m_userRotation = DirectX::XMQuaternionMultiply(local, m_userRotation);
	m_userTranslation = m_userTranslation + offset;
}

void detail::BoneImpl::Transform(const DirectX::XMTRANSFORM& transform, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable)))
		return;

	setDirty();

	m_userRotation = DirectX::XMQuaternionMultiply(transform.GetRotationQuaternion(), m_userRotation);
	m_userTranslation = m_userTranslation + transform.GetTranslation();
}

void detail::BoneImpl::Rotate(DirectX::FXMVECTOR axis, float angle, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable)))
		return;

	setDirty();

	auto local = DirectX::XMQuaternionRotationAxis(axis, angle);

	m_userRotation = DirectX::XMQuaternionMultiply(local, m_userRotation);
}

void detail::BoneImpl::Rotate(DirectX::FXMVECTOR angles, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable)))
		return;

	setDirty();

	auto local = DirectX::XMQuaternionRotationRollPitchYawFromVector(angles);

	m_userRotation = DirectX::XMQuaternionMultiply(local, m_userRotation);
}

void detail::BoneImpl::Translate(DirectX::FXMVECTOR offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable)))
		return;

	setDirty();

	m_userTranslation = m_userTranslation + offset;
}

void detail::BoneImpl::ResetTransform()
{
	setDirty();
	m_userTranslation = DirectX::XMVectorZero();
	m_userRotation = DirectX::XMQuaternionIdentity();
}

void detail::BoneImpl::ApplyPhysicsTransform(DirectX::XMTRANSFORM &transform)
{
	m_transform = m_inverse * transform;
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

		w = DirectX::XMMatrixScaling(0.3f, sqrtf(lensq), 0.3f) *
			DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionMultiply(m_debugRotation, GetTransform().GetRotationQuaternion())) *
			DirectX::XMMatrixTranslationFromVector(GetTransform().GetTranslation());

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
