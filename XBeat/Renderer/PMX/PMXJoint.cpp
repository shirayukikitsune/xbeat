#include "PMXJoint.h"
#include "PMXModel.h"

using namespace Renderer::PMX;

Joint::Joint()
{
}


Joint::~Joint()
{
}

bool Joint::Initialize(std::shared_ptr<Physics::Environment> physics, Model *model, Loader::Joint *joint)
{
	auto pmxBodyA = model->GetRigidBodyById(joint->data.bodyA);
	auto pmxBodyB = model->GetRigidBodyById(joint->data.bodyB);

	if (!pmxBodyA || !pmxBodyB)
		return false;

	btTransform tr, trA, trB;
	btMatrix3x3 bm;
	tr.setIdentity();
	bm.setEulerZYX(joint->data.rotation.x(), joint->data.rotation.y(), joint->data.rotation.z());
	tr.setBasis(bm);
	tr.setOrigin(joint->data.position);
	trA = pmxBodyA->getBody()->getWorldTransform().inverse() * tr;
	trB = pmxBodyB->getBody()->getWorldTransform().inverse() * tr;

	switch (joint->type) {
	case JointType::Spring6DoF:
	{
		auto constraint = new btGeneric6DofSpringConstraint(*pmxBodyA->getBody(), *pmxBodyB->getBody(), trA, trB, true);
		m_constraint.reset(constraint);

		constraint->setLinearLowerLimit(joint->data.lowerMovementRestrictions);
		constraint->setLinearUpperLimit(joint->data.upperMovementRestrictions);
		constraint->setAngularLowerLimit(joint->data.lowerRotationRestrictions);
		constraint->setAngularUpperLimit(joint->data.upperRotationRestrictions);

		for (int i = 0; i < 6; i++) {
			if (i >= 3 || joint->data.springConstant[i] != 0.0f) {
				constraint->enableSpring(i, true);
				constraint->setStiffness(i, joint->data.springConstant[i]);
			}
		}

		break;
	}
	case JointType::SixDoF:
	{
		auto constraint = new btGeneric6DofConstraint(*pmxBodyA->getBody(), *pmxBodyB->getBody(), trA, trB, true);
		m_constraint.reset(constraint);

		constraint->setLinearLowerLimit(joint->data.lowerMovementRestrictions);
		constraint->setLinearUpperLimit(joint->data.upperMovementRestrictions);
		constraint->setAngularLowerLimit(joint->data.lowerRotationRestrictions);
		constraint->setAngularUpperLimit(joint->data.upperRotationRestrictions);
		break;
	}
	case JointType::PointToPoint:
		m_constraint.reset(new btPoint2PointConstraint(*pmxBodyA->getBody(), *pmxBodyB->getBody(), trA.getOrigin(), trB.getOrigin()));
		break;
	case JointType::ConeTwist:
	{
		auto constraint = new btConeTwistConstraint(*pmxBodyA->getBody(), *pmxBodyB->getBody(), trA, trB);
		m_constraint.reset(constraint);

		constraint->setLimit(joint->data.lowerRotationRestrictions.z(), joint->data.lowerRotationRestrictions.y(), joint->data.lowerRotationRestrictions.x(), joint->data.springConstant[0], joint->data.springConstant[1], joint->data.springConstant[2]);
		constraint->setDamping(joint->data.lowerMovementRestrictions.x());
		constraint->setFixThresh(joint->data.upperMovementRestrictions.x());

		bool enableMotor = joint->data.lowerMovementRestrictions.z() != 0.0f;
		constraint->enableMotor(enableMotor);
		if (enableMotor) {
			constraint->setMaxMotorImpulse(joint->data.upperMovementRestrictions.z());
			btQuaternion q;
			q.setEulerZYX(joint->data.springConstant[4], joint->data.springConstant[5], joint->data.springConstant[6]);
			constraint->setMotorTargetInConstraintSpace(q);
		}

		break;
	}
	case JointType::Slider:
	{
		auto constraint = new btSliderConstraint(*pmxBodyA->getBody(), *pmxBodyB->getBody(), trA, trB, true);
		m_constraint.reset(constraint);

		constraint->setLowerLinLimit(joint->data.lowerMovementRestrictions.x());
		constraint->setUpperLinLimit(joint->data.upperMovementRestrictions.x());
		constraint->setLowerAngLimit(joint->data.lowerRotationRestrictions.x());
		constraint->setUpperAngLimit(joint->data.upperRotationRestrictions.x());

		bool poweredLinearMotor = joint->data.springConstant[0] != 0.0f;
		constraint->setPoweredLinMotor(poweredLinearMotor);
		if (poweredLinearMotor) {
			constraint->setTargetLinMotorVelocity(joint->data.springConstant[1]);
			constraint->setMaxLinMotorForce(joint->data.springConstant[2]);
		}

		bool poweredAngularMotor = joint->data.springConstant[3] != 0.0f;
		constraint->setPoweredAngMotor(poweredAngularMotor);
		if (poweredAngularMotor) {
			constraint->setTargetAngMotorVelocity(joint->data.springConstant[4]);
			constraint->setMaxAngMotorForce(joint->data.springConstant[5]);
		}
		break;
	}
	case JointType::Hinge:
	{
		auto constraint = new btHingeConstraint(*pmxBodyA->getBody(), *pmxBodyB->getBody(), trA, trB, true);
		m_constraint.reset(constraint);

		constraint->setLimit(joint->data.lowerRotationRestrictions.x(), joint->data.upperRotationRestrictions.x(), joint->data.springConstant[0], joint->data.springConstant[1], joint->data.springConstant[2]);

		bool motor = joint->data.springConstant[3] != 0.0f;
		constraint->enableMotor(motor);
		if (motor) {
			constraint->enableAngularMotor(motor, joint->data.springConstant[4], joint->data.springConstant[5]);
		}

		break;
	}
	}

	physics->AddConstraint(m_constraint);

	return true;
}

void Joint::Shutdown(std::shared_ptr<Physics::Environment> physics)
{
	physics->RemoveConstraint(m_constraint);
	m_constraint.reset();
}

