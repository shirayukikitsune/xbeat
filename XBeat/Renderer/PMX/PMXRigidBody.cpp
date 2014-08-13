#include "PMXRigidBody.h"
#include "PMXModel.h"
#include "PMXLoader.h"
#include "PMXBone.h"

using namespace Renderer::PMX;

KinematicMotionState::KinematicMotionState(const btTransform &boneTrans, Bone *bone)
{
	m_bone = bone;
	m_transform = boneTrans;
}

KinematicMotionState::~KinematicMotionState()
{
}

void KinematicMotionState::getWorldTransform(btTransform &worldTrans) const
{
	worldTrans = (m_bone ? m_transform * (btTransform)m_bone->GetTransform() : m_transform);
}

void KinematicMotionState::setWorldTransform(const btTransform &transform)
{
}

RigidBody::RigidBody()
{
}


RigidBody::~RigidBody()
{
}

void RigidBody::Initialize(ID3D11DeviceContext *context, std::shared_ptr<Physics::Environment> physics, Model *model, Loader::RigidBody *body)
{
	btVector3 inertia;

	switch (body->shape) {
	case RigidBodyShape::Box:
		m_shape.reset(new btBoxShape(body->size));
		m_primitive = DirectX::GeometricPrimitive::CreateCube(context);
		break;
	case RigidBodyShape::Sphere:
		m_shape.reset(new btSphereShape(body->size.x()));
		m_primitive = DirectX::GeometricPrimitive::CreateSphere(context);
		break;
	case RigidBodyShape::Capsule:
		m_shape.reset(new btCapsuleShape(body->size.y(), body->size.x()));
		m_primitive = DirectX::GeometricPrimitive::CreateCylinder(context);
		break;
	}

	switch (body->group) {
	case 0: m_color = DirectX::Colors::White; break;
	case 1: m_color = DirectX::Colors::Blue; break;
	case 2: m_color = DirectX::Colors::Red; break;
	case 3: m_color = DirectX::Colors::Green; break;
	case 4: m_color = DirectX::Colors::Yellow; break;
	case 5: m_color = DirectX::Colors::Orange; break;
	case 6: m_color = DirectX::Colors::Black; break;
	case 7: m_color = DirectX::Colors::Magenta; break;
	case 8: m_color = DirectX::Colors::Cyan; break;
	case 9: m_color = DirectX::Colors::Gray; break;
	case 10: m_color = DirectX::Colors::LightBlue; break;
	case 11: m_color = DirectX::Colors::LightPink; break;
	case 12: m_color = DirectX::Colors::LightGreen; break;
	case 13: m_color = DirectX::Colors::LightYellow; break;
	case 14: m_color = DirectX::Colors::LightSalmon; break;
	case 15: m_color = DirectX::Colors::LightCoral; break;
	}

	if (body->mode != RigidBodyMode::Static)
		m_shape->calculateLocalInertia(body->mass, inertia);
	else inertia.setZero();

	if (body->targetBone != -1)
		m_bone = model->GetBoneById(body->targetBone);
	else m_bone = nullptr;

	btMatrix3x3 bm;
	bm.setEulerZYX(body->rotation.x(), body->rotation.y(), body->rotation.z());
	m_transform.setBasis(bm);
	m_transform.setOrigin(body->position);
	m_inverseTransform = m_transform.inverse();

	if (body->mode == RigidBodyMode::Static) {
		m_motion.reset(new KinematicMotionState(m_transform, m_bone));
	}
	else {
		m_motion.reset(new btDefaultMotionState(m_transform));
		//m_kinematic.reset(new KinematicMotionState(m_transform, m_bone));
	}

	btRigidBody::btRigidBodyConstructionInfo ci(body->mass, m_motion.get(), m_shape.get(), inertia);
	ci.m_friction = body->friction;
	ci.m_mass = (body->mode == RigidBodyMode::Static ? 0.0f : body->mass);
	ci.m_restitution = body->restitution;
	ci.m_angularDamping = body->angularDamping;
	ci.m_linearDamping = body->linearDamping;
	ci.m_additionalDamping = true;

	m_body.reset(new btRigidBody(ci));
	m_name = body->name;

	if (body->mode == RigidBodyMode::Static)
		m_body->setCollisionFlags(m_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);

	m_body->setActivationState(DISABLE_DEACTIVATION);

	m_groupId = 1 << body->group;
	m_groupMask = body->groupMask;
	m_mode = body->mode;
	m_size = body->size;
	m_shapeType = body->shape;

	physics->AddRigidBody(m_body, m_groupId, m_groupMask);
}

bool XM_CALLCONV RigidBody::Render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	DirectX::XMMATRIX w;
	DirectX::XMVECTOR scale;

	if (m_primitive)
	{
		switch (m_shapeType) {
		case RigidBodyShape::Box:
			scale = DirectX::XMVectorSet(m_size.x(), m_size.y(), m_size.z(), 1.0f);
			break;
		case RigidBodyShape::Sphere:
			scale = DirectX::XMVectorSet(m_size.x(), m_size.x(), m_size.x(), 1.0f);
			break;
		case RigidBodyShape::Capsule:
			scale = DirectX::XMVectorSet(m_size.x(), m_size.y(), m_size.x(), 1.0f);
			break;
		}
		auto tr = m_body->getCenterOfMassTransform();
		
		w = DirectX::XMMatrixAffineTransformation(scale, DirectX::XMVectorZero(), tr.getRotation().get128(), tr.getOrigin().get128());

		m_primitive->Draw(w, view, projection, m_color);
	}

	return true;
}

void RigidBody::Update()
{
	if (m_mode == RigidBodyMode::Static || m_bone == nullptr)
		return;

	btTransform tr = m_body->getCenterOfMassTransform();
	tr *= m_inverseTransform;
	if (m_mode == RigidBodyMode::AlignedDynamic) {
		tr.setOrigin(btVector3(0, 0, 0));
	}
	m_bone->ApplyPhysicsTransform((DirectX::XMTRANSFORM)tr);
}

RigidBody::operator btRigidBody*()
{
	return m_body.get();
}