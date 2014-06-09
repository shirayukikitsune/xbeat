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

	btVector3 direction = GetEndPosition() - GetPosition();
	btVector3 up(0.0f, 1.0f, 0.0f);

	btVector3 axis = up.cross(direction);
	if (!axis.isZero()) {
		// if the axis is the null vector, it means that they are linearly dependent (direction = K * up), no rotation.
		// cos(O) = (a . b) / ||a||.||b||
		btScalar angle = btAcos(up.dot(direction) / direction.length());

		m_initialRotation.setRotation(axis, angle);
	}
	else m_initialRotation = btQuaternion::getIdentity();

	m_toOriginTransform.setOrigin(startPosition);
	m_toOriginTransform.setRotation(m_initialRotation);
	m_toOriginTransform = m_toOriginTransform.inverse();

	// If we are an IK bone, then contruct the chain

}

void Bone::Reset()
{
	m_primitive.reset();
	m_transform.setIdentity();
	m_inheritTransform.setIdentity();
	m_toOriginTransform.setIdentity();
	m_userTransform.setIdentity();
	m_morphTransform.setIdentity();
	m_transform.setOrigin(startPosition);
}

btMatrix3x3 Bone::GetLocalAxis()
{
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

Position Bone::GetPosition()
{
	return m_transform(startPosition);
}

Position Bone::GetOffsetPosition()
{
	if (parent == -1)
		return GetPosition();
	
	return GetPosition() - GetParentBone()->GetPosition();
}

Position Bone::GetEndPosition()
{
	Position p;

	if (HasAnyFlag(BoneFlags::Attached)) {
		Bone *other = model->GetBoneById(this->size.attachTo);
		p.setX(other->startPosition.getX());
		p.setY(other->startPosition.getY());
		p.setZ(other->startPosition.getZ());
	}
	else {
		p = this->GetPosition();
		p.setX(this->size.length[0] + p.x());
		p.setY(this->size.length[1] + p.y());
		p.setZ(this->size.length[2] + p.z());
	}

	return m_transform(p);
}

btQuaternion Bone::GetRotation()
{
	return m_initialRotation * m_transform.getRotation();
}

bool Bone::Update(bool force)
{
	if (!force && !m_dirty) return false;

	btVector3 position(0, 0, 0);
	btQuaternion rotation;
	GetLocalAxis().getRotation(rotation);
	
	// Work on translation
	Bone *parent = HasAnyFlag(BoneFlags::InheritTranslation) ? model->GetBoneById(this->inherit.from) : (this->parent != -1 ? GetParentBone() : nullptr);
	if (parent)
	{
		parent->Update();
		if (HasAnyFlag(BoneFlags::LocalInheritance))
			position += parent->getLocalTransform().getOrigin();
		else if (HasAnyFlag(BoneFlags::InheritTranslation)) 
			position += parent->m_inheritTransform.getOrigin() * this->inherit.rate;
		else position += parent->m_inheritTransform.getOrigin() + parent->m_morphTransform.getOrigin();
	}

	parent = HasAnyFlag(BoneFlags::InheritRotation) ? model->GetBoneById(this->inherit.from) : (this->parent != -1 ? GetParentBone() : nullptr);
	if (parent) {
		parent->Update();
		if (HasAnyFlag(BoneFlags::LocalInheritance))
			rotation = parent->getLocalTransform().getRotation();
		else if (HasAnyFlag(BoneFlags::InheritRotation))
			rotation = this->slerp(this->inherit.rate, parent->m_inheritTransform.getRotation(), btQuaternion::getIdentity());
		else rotation = parent->m_inheritTransform.getRotation() * parent->m_morphTransform.getRotation();
	}

	m_inheritTransform.setOrigin(position);
	m_inheritTransform.setRotation(rotation);

	position += m_userTransform.getOrigin() + m_morphTransform.getOrigin();
	rotation = rotation * m_userTransform.getRotation() * m_morphTransform.getRotation();

	m_transform.setOrigin(position);
	m_transform.setRotation(rotation);

	m_dirty = false;
	return true;
}

void Bone::Transform(const btVector3& angles, const btVector3& offset, DeformationOrigin origin)
{
	Rotate(angles, origin);
	Translate(offset, origin);
}

void Bone::Rotate(const btVector3& angles, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable)))
		return;

	m_dirty = true;

	btQuaternion q;
	q.setEuler(angles.x(), angles.y(), angles.z());

	// Get the rotation component
	if (origin == DeformationOrigin::User)
		m_userTransform.setRotation(m_userTransform.getRotation() * q);
	else
		m_transform.setRotation(m_transform.getRotation() * q);
}

void Bone::Translate(const btVector3& offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !HasAllFlags((BoneFlags)((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable)))
		return;

	m_dirty = true;

	if (origin == DeformationOrigin::User)
		m_userTransform.setOrigin(m_userTransform.getOrigin() + offset);
	else
		m_transform.setOrigin(m_transform.getOrigin() + offset);
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
	m_morphTransform.setIdentity();
	for (auto &pair : appliedMorphs) {
		for (auto &data : pair.first->data) {
			if (data.bone.index == this->id) {
				m_morphTransform.setOrigin(m_morphTransform.getOrigin() + (btVector3(data.bone.movement[0], data.bone.movement[1], data.bone.movement[2]) * pair.second));
				m_morphTransform.setRotation(this->slerp(pair.second, m_morphTransform.getRotation(), btQuaternion(data.bone.rotation[0], data.bone.rotation[1], data.bone.rotation[2], data.bone.rotation[3])));
			}
		}
	}
}

// http://www.bulletphysics.org/Bullet/phpBB3/viewtopic.php?f=9&t=9632
btQuaternion Bone::slerp(btScalar fT, const btQuaternion &rkP, const btQuaternion &rkQ, bool shortestPath)
{
	btScalar fCos = rkP.dot(rkQ);
	btQuaternion rkT;

	// Is rotation inversion needed?
	if (fCos < 0.0f && shortestPath)  {
		fCos = -fCos;
		rkT = -rkQ;
	}
	else rkT = rkQ;

	if (fabs(fCos) < 1.f - SIMD_EPSILON) {
		btScalar fSin = btSqrt(1.f - fCos * fCos);
		btScalar fAngle = btAtan2(fSin, fCos);
		btScalar fCsc = 1.f / fSin;
		btScalar fCoeff0 = btSin((1.f - fT) * fAngle) * fCsc;
		btScalar fCoeff1 = btSin(fT * fAngle) * fCsc;
		return rkP * fCoeff0 + rkT * fCoeff1;
	}
	else {
		btQuaternion t = rkP * (1.0f - fT) + rkT * fT;
		return t.normalize();
	}
}

void Bone::updateChildren()
{
	updateVertices();

	for (auto &child : children) {
		child->Update(true);
		child->updateChildren();
	}
}

void Bone::updateVertices()
{
	static btVector3 zero(0, 0, 0);
	static auto getScaling = [this](Vertex *v) {
		if (v->weightMethod == VertexWeightMethod::SDEF) return 0.5f;
		if (v->weightMethod == VertexWeightMethod::BDEF1) return 1.0f;
		float scale = 1.0f;
		for (auto &i : v->boneInfo.BDEF.boneIndexes) {
			if (i != -1 && model->GetBoneById(i) != nullptr)
				scale -= 0.25f;
		}
		return scale;
	};

	for (auto &vertex : vertices) {
		btVector3 localPosition = m_toOriginTransform(vertex.first->position);
		float weight;
		switch (vertex.first->weightMethod) {
		case VertexWeightMethod::SDEF:
			weight = vertex.first->boneInfo.SDEF.weightBias;
			if (vertex.first->boneInfo.SDEF.boneIndexes[1] == this->id) weight = 1.0f - weight;
			break;
		default:
			// QDEF shares the same structure as BDEF, so no need for a special case
			weight = vertex.first->boneInfo.BDEF.weights[vertex.second];
		}

		if (weight > 0.0f) {
			vertex.first->boneOffset[vertex.second] = (m_transform(localPosition) - localPosition) * weight;
			vertex.first->boneRotation[vertex.second] = this->slerp(weight, m_initialRotation, m_transform.getRotation());

			for (auto &m : vertex.first->materials)
				m.first->dirty |= RenderMaterial::DirtyFlags::VertexBuffer;
		}
	}
}

float Bone::getVertexWeight(Vertex *vertex)
{
	int i;

	switch (vertex->weightMethod) {
	case VertexWeightMethod::SDEF:
		if (vertex->boneInfo.SDEF.boneIndexes[0] == this->id)
			return vertex->boneInfo.SDEF.weightBias;
		else if (vertex->boneInfo.SDEF.boneIndexes[1] == this->id)
			return 1.0f - vertex->boneInfo.SDEF.weightBias;
		break;
	default:
		for (i = 0; i < 4; i++) {
			if (vertex->boneInfo.BDEF.boneIndexes[i] == this->id)
				return vertex->boneInfo.BDEF.weights[i];
		}
		break;
	}

	return 0.0f;
}

bool Bone::Render(DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	DirectX::XMMATRIX w;

	if (m_primitive)
	{
		btVector3 len = GetEndPosition() - GetPosition();
		if (len.length2() == 0) {
			return true;
		}

		w = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(0.3f, len.length(), 0.3f, 1.0f), DirectX::XMVectorZero(), GetRotation().get128(), GetPosition().get128());

		m_primitive->Draw(w, view, projection);
	}

	return true;
}
