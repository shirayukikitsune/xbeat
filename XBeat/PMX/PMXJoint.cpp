#include "PMXJoint.h"
#include "PMXModel.h"

using namespace PMX;

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
	bm.setEulerZYX(DirectX::XMConvertToRadians(joint->data.rotation.x), DirectX::XMConvertToRadians(joint->data.rotation.y), DirectX::XMConvertToRadians(joint->data.rotation.z));
	tr.setOrigin(DirectX::XMFloat3ToBtVector3(joint->data.position));
	tr.setBasis(bm);
	trA = pmxBodyA->getBody()->getWorldTransform().inverseTimes(tr);
	trB = pmxBodyB->getBody()->getWorldTransform().inverseTimes(tr);

	switch (joint->type) {
	case JointType::Spring6DoF:
	{
		auto constraint = new btGeneric6DofSpringConstraint(*pmxBodyA->getBody(), *pmxBodyB->getBody(), trA, trB, true);
		m_constraint.reset(constraint);

		constraint->setLinearLowerLimit(DirectX::XMFloat3ToBtVector3(joint->data.lowerMovementRestrictions));
		constraint->setLinearUpperLimit(DirectX::XMFloat3ToBtVector3(joint->data.upperMovementRestrictions));
#if 0
		constraint->setAngularLowerLimit(btVector3(DirectX::XMConvertToRadians(joint->data.lowerRotationRestrictions.x), DirectX::XMConvertToRadians(joint->data.lowerRotationRestrictions.y), DirectX::XMConvertToRadians(joint->data.lowerRotationRestrictions.z)));
		constraint->setAngularUpperLimit(btVector3(DirectX::XMConvertToRadians(joint->data.upperMovementRestrictions.x), DirectX::XMConvertToRadians(joint->data.upperMovementRestrictions.y), DirectX::XMConvertToRadians(joint->data.upperMovementRestrictions.z)));
#else
		constraint->setAngularLowerLimit(DirectX::XMFloat3ToBtVector3(joint->data.lowerRotationRestrictions));
		constraint->setAngularUpperLimit(DirectX::XMFloat3ToBtVector3(joint->data.upperRotationRestrictions));
#endif

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

		constraint->setLinearLowerLimit(DirectX::XMFloat3ToBtVector3(joint->data.lowerMovementRestrictions));
		constraint->setLinearUpperLimit(DirectX::XMFloat3ToBtVector3(joint->data.upperMovementRestrictions));
		constraint->setAngularLowerLimit(DirectX::XMFloat3ToBtVector3(joint->data.lowerRotationRestrictions));
		constraint->setAngularUpperLimit(DirectX::XMFloat3ToBtVector3(joint->data.upperRotationRestrictions));
		break;
	}
	case JointType::PointToPoint:
		m_constraint.reset(new btPoint2PointConstraint(*pmxBodyA->getBody(), *pmxBodyB->getBody(), trA.getOrigin(), trB.getOrigin()));
		break;
	case JointType::ConeTwist:
	{
		auto constraint = new btConeTwistConstraint(*pmxBodyA->getBody(), *pmxBodyB->getBody(), trA, trB);
		m_constraint.reset(constraint);

		constraint->setLimit(joint->data.lowerRotationRestrictions.z, joint->data.lowerRotationRestrictions.y, joint->data.lowerRotationRestrictions.x, joint->data.springConstant[0], joint->data.springConstant[1], joint->data.springConstant[2]);
		constraint->setDamping(joint->data.lowerMovementRestrictions.x);
		constraint->setFixThresh(joint->data.upperMovementRestrictions.x);

		bool enableMotor = joint->data.lowerMovementRestrictions.z != 0.0f;
		constraint->enableMotor(enableMotor);
		if (enableMotor) {
			constraint->setMaxMotorImpulse(joint->data.upperMovementRestrictions.z);
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

		constraint->setLowerLinLimit(joint->data.lowerMovementRestrictions.x);
		constraint->setUpperLinLimit(joint->data.upperMovementRestrictions.x);
		constraint->setLowerAngLimit(joint->data.lowerRotationRestrictions.x);
		constraint->setUpperAngLimit(joint->data.upperRotationRestrictions.x);

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

		constraint->setLimit(joint->data.lowerRotationRestrictions.x, joint->data.upperRotationRestrictions.x, joint->data.springConstant[0], joint->data.springConstant[1], joint->data.springConstant[2]);

		bool motor = joint->data.springConstant[3] != 0.0f;
		constraint->enableMotor(motor);
		if (motor) {
			constraint->enableAngularMotor(motor, joint->data.springConstant[4], joint->data.springConstant[5]);
		}

		break;
	}
	}

	physics->addConstraint(m_constraint);

	m_type = joint->type;

	return true;
}

void Joint::InitializeDebug(ID3D11DeviceContext *context)
{
	m_primitive = DirectX::GeometricPrimitive::CreateCube(context, 0.5f);
}

void Joint::Shutdown(std::shared_ptr<Physics::Environment> physics)
{
	physics->removeConstraint(m_constraint);
	m_constraint.reset();
	m_primitive.reset();
}

void XM_CALLCONV Joint::Render(DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection)
{
	DirectX::XMMATRIX world;
	if (m_type == JointType::Spring6DoF) {
		auto constraint = std::dynamic_pointer_cast<btGeneric6DofSpringConstraint>(m_constraint);
		btVector3 pos = constraint->getCalculatedTransformA().getOrigin() + constraint->getCalculatedTransformB().getOrigin();
		pos *= 0.5f;
		btQuaternion q = constraint->getCalculatedTransformA().getRotation().slerp(constraint->getCalculatedTransformB().getRotation(), 0.5f);
		world = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSplatOne(), DirectX::XMVectorZero(), q.get128(), pos.get128());
		m_primitive->Draw(world, view, projection, DirectX::Colors::Bisque);
	}
}
