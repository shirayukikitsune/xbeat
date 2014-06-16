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
}

Bone::~Bone(void)
{
}

void Bone::Initialize(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	if (!HasAnyFlag(BoneFlags::IK) && HasAnyFlag(BoneFlags::View))
		m_primitive = DirectX::GeometricPrimitive::CreateCylinder(d3d->GetDeviceContext());

	DirectX::XMVECTOR direction = GetEndPosition() - GetPosition();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMVECTOR axis = DirectX::XMVector3Cross(up, direction);
	if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(axis)) != 0) {
		// if the axis is the null vector, it means that they are linearly dependent (direction = K * up), no rotation.
		// cos(O) = (a . b) / ||a||.||b||
		float angle = DirectX::XMVectorGetX(DirectX::XMVectorACos(DirectX::XMVectorDivide(DirectX::XMVector3Dot(up, direction), DirectX::XMVector3Length(direction))));

		m_initialRotation = DirectX::XMQuaternionRotationAxis(DirectX::XMVector3Normalize(axis), angle);
	}
	else m_initialRotation = DirectX::XMQuaternionIdentity();

	// If we are an IK bone, then contruct the chain

}

void Bone::Reset()
{
	m_primitive.reset();
	m_transform = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMVectorZero(), m_initialRotation, DirectX::XMVectorSet(startPosition.x(), startPosition.y(), startPosition.z(), 0.0f));
	m_inheritTransform = m_userTransform = m_morphTransform = DirectX::XMMatrixIdentity();

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

DirectX::XMVECTOR XM_CALLCONV Bone::GetPosition()
{
	return DirectX::XMVector3Transform(startPosition.get128(), m_transform);
}

DirectX::XMVECTOR XM_CALLCONV Bone::GetOffsetPosition()
{
	if (parent == -1)
		return GetPosition();
	
	return GetPosition() - GetParentBone()->GetPosition();
}

DirectX::XMVECTOR XM_CALLCONV Bone::GetEndPosition()
{
	DirectX::XMVECTOR p;

	if (HasAnyFlag(BoneFlags::Attached)) {
		Bone *other = model->GetBoneById(this->size.attachTo);
		p = other->startPosition.get128();
	}
	else {
		p = DirectX::XMVectorSet(this->size.length[0], this->size.length[1], this->size.length[2], 0.0f) + GetPosition();
	}

	return DirectX::XMVector3Transform(p, m_transform);
}

DirectX::XMVECTOR XM_CALLCONV Bone::GetRotation()
{
	return DirectX::XMVector4Normalize(DirectX::XMQuaternionMultiply(m_initialRotation, DirectX::XMQuaternionRotationMatrix(m_transform)));
}

bool Bone::Update(bool force)
{
	if (!force && !m_dirty) return false;

	DirectX::XMVECTOR position = DirectX::XMVectorZero();
	DirectX::XMVECTOR rotation, tmp, unused[2];
	{
		btQuaternion q;
		GetLocalAxis().getRotation(q);
		rotation = q.get128();
	}
	
	// Work on translation
	Bone *parent = HasAnyFlag(BoneFlags::InheritTranslation) ? model->GetBoneById(this->inherit.from) : (this->parent != -1 ? GetParentBone() : nullptr);
	if (parent)
	{
		parent->Update();

		if (HasAnyFlag(BoneFlags::LocalInheritance))
			DirectX::XMMatrixDecompose(&unused[0], &unused[1], &position, parent->getLocalTransform());
		else if (HasAnyFlag(BoneFlags::InheritTranslation))
			DirectX::XMMatrixDecompose(&unused[0], &unused[1], &position, parent->m_inheritTransform * this->inherit.rate);
		else DirectX::XMMatrixDecompose(&unused[0], &unused[1], &position, parent->m_transform);
	}

	parent = HasAnyFlag(BoneFlags::InheritRotation) ? model->GetBoneById(this->inherit.from) : (this->parent != -1 ? GetParentBone() : nullptr);
	if (parent) {
		parent->Update();

		if (HasAnyFlag(BoneFlags::LocalInheritance))
			DirectX::XMMatrixDecompose(&unused[0], &rotation, &unused[1], parent->getLocalTransform());
		else if (HasAnyFlag(BoneFlags::InheritRotation)) {
			DirectX::XMMatrixDecompose(&unused[0], &rotation, &unused[1], parent->m_inheritTransform);
			rotation = DirectX::XMQuaternionSlerp(rotation, DirectX::XMQuaternionIdentity(), this->inherit.rate);
		}
		else DirectX::XMMatrixDecompose(&unused[0], &rotation, &unused[1], parent->m_transform);
	}

	m_inheritTransform = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMVectorZero(), rotation, position);

	m_transform = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(m_inheritTransform, m_userTransform), m_morphTransform);

	m_dirty = false;
	m_touched = true;
	return true;
}

void Bone::Transform(const btVector3& angles, const btVector3& offset, DeformationOrigin origin)
{
	Rotate(angles, origin);
	Translate(offset, origin);
}

void Bone::Rotate(const btVector3& axis, float angle, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable)))
		return;

	m_dirty = true;

	auto local = DirectX::XMMatrixRotationAxis(axis.get128(), angle) - DirectX::XMMatrixIdentity();

	if (origin == DeformationOrigin::User)
		m_userTransform += local;
	else
		m_transform += local;
}

void Bone::Rotate(const btVector3& angles, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable)))
		return;

	m_dirty = true;

	auto local = DirectX::XMMatrixRotationRollPitchYawFromVector(angles.get128()) - DirectX::XMMatrixIdentity();

	if (origin == DeformationOrigin::User)
		m_userTransform += local;
	else
		m_transform += local;
}

void Bone::Translate(const btVector3& offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable)))
		return;

	m_dirty = true;

	auto local = DirectX::XMMatrixTranslationFromVector(offset.get128()) - DirectX::XMMatrixIdentity();

	if (origin == DeformationOrigin::User)
		m_userTransform += local;
	else
		m_transform += local;
}

void Bone::ApplyMorph(Morph *morph, float weight)
{
	auto i = ([this, morph](){ for (auto i = appliedMorphs.begin(); i != appliedMorphs.end(); i++) { if (i->first == morph) return i; } return appliedMorphs.end(); })();
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

	// Now calculate the transformation matrix
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
	m_morphTransform = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMVectorZero(), rotation, position);
}

void Bone::updateChildren()
{
	for (auto &child : children) {
		child->Update(true);
		child->updateChildren();
	}
}

bool Bone::wasTouched()
{
	// Great hack(?) to check if the bone was updated this frame
	bool ret = m_touched;
	m_touched = false;
	return ret;
}

bool XM_CALLCONV Bone::Render(DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection)
{
	DirectX::XMMATRIX w;

	if (m_primitive)
	{
		float lensq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(GetEndPosition() - GetPosition()));
		if (lensq == 0) {
			return true;
		}

		w = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(0.3f, sqrt(lensq), 0.3f, 1.0f), DirectX::XMVectorZero(), GetRotation(), GetPosition());

		m_primitive->Draw(w, view, projection);
	}

	return true;
}
