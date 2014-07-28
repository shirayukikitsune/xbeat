#include "PMXRigidBody.h"
#include "PMXModel.h"
#include "PMXLoader.h"
#include "PMXBone.h"

using namespace Renderer::PMX;

btTransform DirectXMatrixToBtTransform(DirectX::XMTRANSFORM M)
{
	btQuaternion q;
	q.set128(M.GetRotationQuaternion());
	btVector3 p;
	p.set128(M.GetOffset());
	return btTransform(q, p);
}


KinematicMotionState::KinematicMotionState(const btTransform &startTrans, const btTransform &boneTrans, Bone *bone)
{
	m_bone = bone;
	m_transform = boneTrans;
}

KinematicMotionState::~KinematicMotionState()
{
}

void KinematicMotionState::getWorldTransform(btTransform &worldTrans) const
{
	worldTrans = DirectXMatrixToBtTransform(m_bone->getLocalTransform()) * m_transform;
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
	btTransform startTransform;

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

	startTransform.setIdentity();
	btVector3 offset;
	offset.set128(m_bone->GetPosition(false));
	startTransform.setOrigin(offset);
	startTransform *= m_transform;

	if (body->mode == RigidBodyMode::Static) {
		m_motion.reset(new KinematicMotionState(startTransform, m_transform, m_bone));
	}
	else {
		m_motion.reset(new btDefaultMotionState(startTransform));
		m_kinematic.reset(new KinematicMotionState(startTransform, m_transform, m_bone));
	}

	btRigidBody::btRigidBodyConstructionInfo ci(body->mass, m_motion.get(), m_shape.get(), inertia);
	ci.m_friction = body->friction;
	ci.m_mass = (body->mode == RigidBodyMode::Static ? 0.0f : body->mass);
	ci.m_restitution = body->restitution;
	ci.m_angularDamping = body->angularDamping;
	ci.m_linearDamping = body->linearDamping;

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
		
		w = DirectX::XMMatrixAffineTransformation(scale, m_body->getCenterOfMassTransform().getOrigin().get128(), m_body->getCenterOfMassTransform().getRotation().get128(), m_body->getCenterOfMassPosition().get128());

		m_primitive->Draw(world, view, projection);
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
		btVector3 v;
		m_bone->Update();
		v.set128(m_bone->getLocalTransform().GetOffset());
		tr.setOrigin(v);
	}
	m_bone->ApplyPhysicsTransform((DirectX::XMTRANSFORM)tr);
}

RigidBody::operator btRigidBody*()
{
	return m_body.get();
}