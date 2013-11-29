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
}

Bone::~Bone(void)
{
}

void Bone::Initialize(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	if (!HasAnyFlag(BoneFlags::IK))
		m_primitive = DirectX::GeometricPrimitive::CreateCylinder(d3d->GetDeviceContext());

	m_toOriginTransform = m_transform.inverse();
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
	if (!HasAnyFlag(BoneFlags::LocalAxis))
		return btMatrix3x3::getIdentity();

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
	return model->GetBoneById(GetParentId());
}

Position Bone::GetPosition()
{
	return m_transform.getOrigin();
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
		p.setX(other->GetPosition().getX());
		p.setY(other->GetPosition().getY());
		p.setZ(other->GetPosition().getZ());
	}
	else {
		p = this->GetPosition();
		p.setX(this->size.length[0] + p.x());
		p.setY(this->size.length[1] + p.y());
		p.setZ(this->size.length[2] + p.z());
	}

	return p;
}

void Bone::Update()
{
	btVector3 position = startPosition;
	btQuaternion rotation(0,0,0,1);
	
	Bone *parent = HasAnyFlag((BoneFlags::Flags)(BoneFlags::InheritRotation | BoneFlags::InheritTranslation)) ? model->GetBoneById(this->inherit.from) : GetParentBone();

	if (parent) {
		if (HasAnyFlag(BoneFlags::LocalInheritance)) {
			// Local inherited position and rotation
			rotation = parent->getLocalTransform().getRotation();
			position += parent->getLocalTransform().getOrigin();
		}
		else {
			if (HasAnyFlag(BoneFlags::InheritRotation))
				rotation = parent->m_inheritTransform.getRotation() / this->inherit.rate;
			else rotation = parent->m_userTransform.getRotation() * parent->m_morphTransform.getRotation();

			if (HasAnyFlag(BoneFlags::InheritTranslation))
				position += parent->m_inheritTransform.getOrigin() * this->inherit.rate;
			else position += parent->m_userTransform.getOrigin() + parent->m_morphTransform.getOrigin();
		}

		position = parent->m_toOriginTransform(position);
	}

	m_inheritTransform.setOrigin(position);
	m_inheritTransform.setRotation(rotation);

	position += m_userTransform.getOrigin() + m_morphTransform.getOrigin();
	rotation = rotation * m_userTransform.getRotation() * m_morphTransform.getRotation();

	m_transform.setOrigin(position);
	m_transform.setRotation(rotation);
}

void Bone::Transform(const btVector3& angles, const btVector3& offset, DeformationOrigin::Id origin)
{
	Rotate(angles, origin);
	Translate(offset, origin);
}

void Bone::Rotate(const btVector3& angles, DeformationOrigin::Id origin)
{
	btQuaternion q;
	q.setEulerZYX(angles.x(), angles.y(), angles.z());

	// Get the rotation component
	btQuaternion rotation = this->m_userTransform.getRotation();
	if (origin == DeformationOrigin::User)
		m_userTransform.setRotation(m_userTransform.getRotation() * q);

	Update();

	updateChildren();
}

void Bone::Translate(const btVector3& offset, DeformationOrigin::Id origin)
{
	if (origin == DeformationOrigin::User)
		m_userTransform.setOrigin(m_userTransform.getOrigin() + offset);

	Update();

	updateChildren();
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

	// Now calculate the transformation matrix
	btTransform current;
	m_morphTransform.setIdentity();
	for (auto &pair : appliedMorphs) {
		for (auto &data : pair.first->data) {
			if (data.bone.index == this->id) {
				current.setOrigin(btVector3(data.bone.movement[0], data.bone.movement[1], data.bone.movement[2]) * pair.second);
				current.setRotation(btQuaternion::getIdentity().slerp(btQuaternion(data.bone.rotation[0], data.bone.rotation[1], data.bone.rotation[2], data.bone.rotation[3]), pair.second));
				m_morphTransform *= current;
			}
		}
	}

	Update();
	updateChildren();
}

void Bone::updateChildren()
{
	updateVertices();

	for (auto &child : children) {
		child->Update();
		child->updateChildren();
	}
}

float getScaling(Vertex *v, Model *m)
{
	if (v->weightMethod == VertexWeightMethod::SDEF) return 0.5f;
	if (v->weightMethod == VertexWeightMethod::BDEF1) return 1.0f;
	float scale = 1.0f;
	for (auto &i : v->boneInfo.BDEF.boneIndexes) {
		if (i != -1 && m->GetBoneById(i) != nullptr)
			scale -= 0.25f;
	}
	return scale;
}

void Bone::updateVertices()
{
	static btVector3 zero(0, 0, 0);
	for (auto &vertex : vertices) {
		btVector3 localPosition = m_toOriginTransform(vertex.first->position);
		float weight;
		switch (vertex.first->weightMethod) {
		case VertexWeightMethod::QDEF:
			weight = vertex.first->boneInfo.BDEF.weights[vertex.second];
			//offset = quatRotate(btQuaternion::getIdentity().slerp(m_transform.getRotation(), weight), localPosition + m_transform.getOrigin() * weight);
			break;
		case VertexWeightMethod::SDEF:
			weight = vertex.first->boneInfo.SDEF.weightBias;
			if (vertex.first->boneInfo.SDEF.boneIndexes[1] == this->id) weight = 1.0f - weight;
			break;
		default:
			weight = vertex.first->boneInfo.BDEF.weights[vertex.second];
		}
		vertex.first->boneOffset[vertex.second] = (m_transform(localPosition) * weight);// *getScaling(vertex.first, model);
		vertex.first->boneRotation[vertex.second] = btQuaternion::getIdentity().slerp(m_transform.getRotation(), weight);

		if (vertex.first->material)
			vertex.first->material->dirty |= RenderMaterial::DirtyFlags::VertexBuffer;
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

bool Bone::Render(std::shared_ptr<Renderer::D3DRenderer> d3d, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	DirectX::XMMATRIX w;

	if (m_primitive)
	{
		btVector3 len = this->GetEndPosition() - this->GetPosition();
		if (len.length2() == 0) {
			return true;
		}

		if (len.x() == 0) len.setX(1.0f);
		if (len.y() == 0) len.setY(1.0f);
		if (len.z() == 0) len.setZ(1.0f);

		//w = DirectX::XMMatrixRotationZ(len.x()) * DirectX::XMMatrixRotationY(len.y()) * DirectX::XMMatrixRotationX(len.z());
		w = DirectX::XMMatrixScalingFromVector(len.get128()) *
			DirectX::XMMatrixRotationRollPitchYawFromVector(len.normalized().get128()) *
			DirectX::XMMatrixTranslationFromVector(m_transform.getOrigin().get128());
		//w = DirectX::XMMatrixRotationAxisNormal(DirectX::XMVector3Normalize(len.get128()), DirectX::g_XMTwoPi.f[0]);
		//w.r[0] = DirectX::XMVectorScale(w.r[0], len.x());
		//w.r[1] = DirectX::XMVectorScale(w.r[1], len.y());
		//w.r[2] = DirectX::XMVectorScale(w.r[2], len.z());
		w.r[3] = DirectX::XMVectorSetW(m_transform.getOrigin().get128(), 1.0f);
		/*w = DirectX::XMMatrixTransformation(DirectX::XMVectorZero(), DirectX::XMQuaternionIdentity(), len.get128(),
			DirectX::XMVectorZero(), m_transform.getRotation().get128(),
			m_transform.getOrigin().get128());*/

		m_primitive->Draw(w, view, projection);
	}

	return true;
}
