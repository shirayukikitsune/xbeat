#include "PMXBone.h"
#include "PMXModel.h"
#include "PMXMaterial.h"
#include <cfloat>
#include <algorithm>

using namespace Renderer::PMX;

Bone::Bone(Model *model, uint32_t id)
	: startPosition(0,0,0)
{
	Reset();
	this->model = model;
	this->id = id;
	m_dirty = false;
	m_touched = true;
	this->inherit.rate = 1.0f;
}

Bone::~Bone(void)
{
}

void Bone::Initialize(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	startPosition.setW(1.0f);

	if (!HasAnyFlag(BoneFlags::IK) && HasAnyFlag(BoneFlags::View))
		m_primitive = DirectX::GeometricPrimitive::CreateCylinder(d3d->GetDeviceContext());

	m_initialTransformInverse.SetTranslation((-startPosition).get128());
	//m_transform.SetTranslation(startPosition.get128());
}

void Bone::Reset()
{
	m_primitive.reset();
	m_inheritTranslation = m_userTranslation = m_morphTranslation = DirectX::XMVectorZero();
	m_inheritRotation = m_userRotation = m_morphRotation = DirectX::XMQuaternionIdentity();
}

btMatrix3x3 Bone::GetLocalAxis()
{
	if (this->id == -1)
		return btMatrix3x3::getIdentity();

	if (!HasAnyFlag(BoneFlags::LocalAxis)) {
		auto parent = GetParentBone();
		return (parent && parent->id != this->id ? parent->GetLocalAxis() : btMatrix3x3::getIdentity());
	}

	btVector3 X = localAxes.xDirection;
	btVector3 Z = localAxes.zDirection;
	btVector3 Y = Z.cross(X);
	Z = X.cross(Y);

	return btMatrix3x3(X.x(), X.y(), X.z(),
		               Y.x(), Y.y(), Y.z(),
					   Z.x(), Z.y(), Z.z());
}

Bone* Bone::GetParentBone()
{
	if (GetParentId() == -1) {
		return model->GetRootBone();
	}

	return model->GetBoneById(GetParentId());
}

DirectX::XMVECTOR XM_CALLCONV Bone::GetPosition(bool transform)
{
	if (!transform)
		return startPosition.get128();

	return m_transform.PointTransform(startPosition.get128());
}

DirectX::XMVECTOR XM_CALLCONV Bone::GetOffsetPosition()
{
	//if (parent == -1)
		return startPosition.get128();
	
	//return startPosition.get128() - GetParentBone()->startPosition.get128();
}

DirectX::XMVECTOR XM_CALLCONV Bone::GetEndPosition(bool transform)
{
	DirectX::XMVECTOR p;

	if (HasAnyFlag(BoneFlags::Attached)) {
		Bone *other = model->GetBoneById(this->size.attachTo);
		p = other->startPosition.get128();
	}
	else {
		p = DirectX::XMVectorSet(this->size.length[0], this->size.length[1], this->size.length[2], 0.0f) + GetPosition();
	}

	if (!transform)
		return p;

	return m_transform.PointTransform(p);
}

DirectX::XMVECTOR XM_CALLCONV Bone::GetRotation()
{
	return m_transform.GetRotationQuaternion();
}

bool Bone::Update(bool force)
{
	if (!force && !m_dirty) return false;

	DirectX::XMVECTOR translation = DirectX::XMVectorZero();
	
	// Work on translation
	if (HasAnyFlag(BoneFlags::LocalInheritance))
		translation = GetParentBone()->getLocalTransform().GetTranslation();
	else {
		DirectX::XMVECTOR tmp = DirectX::XMVectorZero();
		if (HasAnyFlag(BoneFlags::InheritTranslation)) {
			tmp = model->GetBoneById(this->inherit.from)->m_inheritTranslation * this->inherit.rate;
		}
		translation = m_userTranslation + m_morphTranslation + tmp;
	}

	// Work on rotation..
	DirectX::XMVECTOR rotation = DirectX::XMQuaternionIdentity();
	if (HasAnyFlag(BoneFlags::LocalInheritance))
		rotation = GetParentBone()->GetRotation();
	else if (HasAnyFlag(BoneFlags::InheritRotation))
		rotation = DirectX::XMQuaternionMultiply(DirectX::XMQuaternionMultiply(m_userRotation, m_morphRotation), DirectX::XMQuaternionSlerp(DirectX::XMQuaternionIdentity(), model->GetBoneById(this->inherit.from)->m_inheritRotation, this->inherit.rate));
	else
		rotation = DirectX::XMQuaternionMultiply(m_userRotation, m_morphRotation);

	rotation = DirectX::XMQuaternionNormalize(rotation);

	m_inheritRotation = rotation;
	m_inheritTranslation = translation;

	DirectX::XMVECTOR origin = translation + GetOffsetPosition();
	m_transform = DirectX::XMTRANSFORM(origin, rotation, translation);
	if (auto parent = GetParentBone()) {
		m_transform.PrependTransform(parent->m_transform);
	}

	m_dirty = false;
	m_touched = true;
	return true;
}

void Bone::Transform(const btVector3& angles, const btVector3& offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable)))
		return;

	m_dirty = true;

	auto local = DirectX::XMQuaternionRotationRollPitchYawFromVector(angles.get128());

	m_userRotation = DirectX::XMQuaternionMultiply(local, m_userRotation);
	m_userTranslation = m_userTranslation + offset.get128();

	tagChildren();
}

void Bone::Rotate(const btVector3& axis, float angle, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable)))
		return;

	m_dirty = true;

	auto local = DirectX::XMQuaternionRotationAxis(axis.get128(), angle);

	m_userRotation = DirectX::XMQuaternionMultiply(local, m_userRotation);

	tagChildren();
}

void Bone::Rotate(const btVector3& angles, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable)))
		return;

	m_dirty = true;

	auto local = DirectX::XMQuaternionRotationRollPitchYawFromVector(angles.get128());

	m_userRotation = DirectX::XMQuaternionMultiply(local, m_userRotation);

	tagChildren();
}

void Bone::Translate(const btVector3& offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable)))
		return;

	m_dirty = true;

	m_userTranslation = m_userTranslation + offset.get128();

	tagChildren();
}

void Bone::ResetTransform()
{
	m_dirty = true;
	m_userTranslation = DirectX::XMVectorZero();
	m_userRotation = DirectX::XMQuaternionIdentity();

	tagChildren();
}

void Bone::ApplyMorph(Morph *morph, float weight)
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
			if (data.bone.index == this->id) {
				tmp = pair.second * DirectX::XMVectorSet(data.bone.movement[0], data.bone.movement[1], data.bone.movement[2], 0.0f);
				position = position + tmp;
				rotation = DirectX::XMQuaternionSlerp(rotation, DirectX::XMVectorSet(data.bone.rotation[0], data.bone.rotation[1], data.bone.rotation[2], data.bone.rotation[3]), pair.second);
			}
		}
	}
	m_morphRotation = rotation;
	m_morphTranslation = position;

	tagChildren();
}

void Bone::updateChildren()
{
	for (auto &child : children) {
		child->Update(true);
		child->updateChildren();
	}
}

void Bone::tagChildren()
{
	if (id == -1) return;

	for (auto &child : children) {
		child->m_dirty = true;
		child->tagChildren();
	}
}

bool Bone::wasTouched()
{
	// Great hack(?) to check if the bone was updated this frame
	bool ret = m_touched;
	m_touched = false;
	return ret;
}

DirectX::XMTRANSFORM XM_CALLCONV Bone::getLocalTransform() const
{
	return DirectX::XMMatrixMultiply(m_transform, m_initialTransformInverse);
}

bool XM_CALLCONV Bone::Render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	DirectX::XMMATRIX w;

	if (m_primitive)
	{
		float lensq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(GetEndPosition() - GetPosition()));
		if (lensq == 0) {
			return true;
		}

		w = (DirectX::XMMATRIX)m_transform * world * DirectX::XMMatrixScaling(0.3f, sqrtf(lensq), 0.3f);

		m_primitive->Draw(w, view, projection);
	}

	return true;
}
