#include "PMXRigidBody.h"
#include "PMXModel.h"
#include "PMXLoader.h"
#include "PMXBone.h"

using namespace Renderer::PMX;

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
	worldTrans = m_bone->getLocalTransform() * m_transform;
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

void RigidBody::Initialize(std::shared_ptr<Physics::Environment> physics, Model *model, Loader::RigidBody *body)
{
	btVector3 inertia;
	btTransform startTransform;

	switch (body->shape) {
	case RigidBodyShape::Box:
		m_shape.reset(new btBoxShape(body->size));
		break;
	case RigidBodyShape::Sphere:
		m_shape.reset(new btSphereShape(body->size.x()));
		break;
	case RigidBodyShape::Capsule:
		return;
		m_shape.reset(new btCapsuleShape(body->size.y(), body->size.x()));
		break;
	}

	if (body->mode != RigidBodyMode::Static)
		m_shape->calculateLocalInertia(body->mass, inertia);

	if (body->targetBone != -1)
		m_bone = model->GetBoneById(body->targetBone);
	else m_bone = model->GetRootBone();

	btVector3 position = m_bone->GetPosition();

	startTransform.setIdentity();
	startTransform.setOrigin(m_bone->getLocalTransform().getOrigin());
	startTransform *= m_transform;

	if (body->mode == RigidBodyMode::Static)
		m_motion.reset(new KinematicMotionState(startTransform, m_bone->getLocalTransform(), m_bone));
	else m_motion.reset(new btDefaultMotionState(startTransform));

	btRigidBody::btRigidBodyConstructionInfo ci(body->mass, m_motion.get(), m_shape.get(), inertia);
	ci.m_friction = body->friction;
	ci.m_mass = body->mass;
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

	physics->AddRigidBody(m_body, m_groupId, m_groupMask);
}
