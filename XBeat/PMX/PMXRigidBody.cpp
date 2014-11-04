#include "PMXRigidBody.h"
#include "PMXModel.h"
#include "PMXLoader.h"
#include "PMXBone.h"
#include "../Physics/PMXMotionState.h"

using namespace PMX;

RigidBody::RigidBody()
{
}


RigidBody::~RigidBody()
{
}

void RigidBody::Initialize(ID3D11DeviceContext *context, std::shared_ptr<Physics::Environment> physics, Model *model, Loader::RigidBody *body)
{
	btVector3 inertia;

	m_size = DirectX::XMFloat3ToBtVector3(body->size);

	switch (body->shape) {
	case RigidBodyShape::Box:
		m_shape.reset(new btBoxShape(m_size));
		m_primitive = DirectX::GeometricPrimitive::CreateCube(context);
		break;
	case RigidBodyShape::Sphere:
		m_shape.reset(new btSphereShape(body->size.x));
		m_primitive = DirectX::GeometricPrimitive::CreateSphere(context);
		break;
	case RigidBodyShape::Capsule:
		m_shape.reset(new btCapsuleShape(body->size.x, body->size.y));
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

	float mass;
	if (body->mode != RigidBodyMode::Static) {
		m_shape->calculateLocalInertia(body->mass, inertia);
		mass = body->mass;
	}
	else {
		inertia.setZero();
		mass = 0.0f;
	}

	if (body->targetBone != -1) {
		m_bone = model->GetBoneById(body->targetBone);
		if (m_bone && body->mode != RigidBodyMode::Static) m_bone->setSimulated();
	}
	else m_bone = nullptr;

	btMatrix3x3 bm;
	bm.setEulerZYX(DirectX::XMConvertToRadians(body->rotation.x), DirectX::XMConvertToRadians(body->rotation.y), DirectX::XMConvertToRadians(body->rotation.z));
	m_transform.setOrigin(DirectX::XMFloat3ToBtVector3(body->position));
	m_transform.setBasis(bm);

	if (body->mode == RigidBodyMode::Static) {
		if (m_bone) m_motion.reset(new Physics::PMXMotionState(m_bone, m_transform));
	}
	else {
		m_motion.reset(new btDefaultMotionState(m_transform));
	}

	btRigidBody::btRigidBodyConstructionInfo ci(mass, m_motion.get(), m_shape.get(), inertia);
	ci.m_friction = body->friction;
	ci.m_restitution = body->restitution;
	ci.m_angularDamping = body->angularDamping;
	ci.m_linearDamping = body->linearDamping;
	//ci.m_startWorldTransform = m_transform;

	m_body.reset(new btRigidBody(ci));
	m_name = body->name;

	if (body->mode == RigidBodyMode::Static)
		m_body->setCollisionFlags(m_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);

	m_body->setActivationState(DISABLE_DEACTIVATION);

	m_groupId = 1 << body->group;
	m_groupMask = body->groupMask;
	m_mode = body->mode;
	m_shapeType = body->shape;
	m_inverse = m_transform.inverse();

	physics->addRigidBody(m_body, m_groupId, m_groupMask);
}

bool XM_CALLCONV RigidBody::Render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	DirectX::XMMATRIX w;

	if (m_primitive)
	{
		auto tr = m_body->getCenterOfMassTransform();
		
		w = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSplatOne(), DirectX::XMVectorZero(), tr.getRotation().get128(), tr.getOrigin().get128());

		m_primitive->Draw(w, view, projection, m_color);
	}

	return true;
}

void RigidBody::Update()
{
	if (m_mode == RigidBodyMode::Static || m_bone == nullptr)
		return;

	btTransform tr = m_body->getCenterOfMassTransform();
	switch (m_mode) {
	case RigidBodyMode::AlignedDynamic: {
		tr.setOrigin(m_bone->GetTransform().getOrigin());
		break;
	}
	}

	m_bone->ApplyPhysicsTransform(tr);
}

RigidBody::operator btRigidBody*()
{
	return m_body.get();
}