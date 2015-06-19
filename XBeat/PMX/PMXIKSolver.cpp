#include "PMXIKSolver.h"

#include "PMXIKNode.h"

#include <Context.h>
#include <Node.h>
#include <Variant.h>

using namespace Urho3D;

PMXIKSolver::PMXIKSolver(Urho3D::Context *context)
	: Urho3D::LogicComponent(context)
{
	threshold = 0.000001f;
	chainLength = 0.0f;
	angleLimit = M_PI;
	loopCount = 20;

	SetUpdateEventMask(USE_FIXEDUPDATE);
}

PMXIKSolver::~PMXIKSolver()
{
}

void PMXIKSolver::RegisterObject(Urho3D::Context *context)
{
	context->RegisterFactory<PMXIKSolver>();

	ATTRIBUTE(PMXIKSolver, VAR_FLOAT, "Threshold", threshold, 0.000001f, AM_DEFAULT);
	ATTRIBUTE(PMXIKSolver, VAR_FLOAT, "Angle Limit", angleLimit, M_PI, AM_DEFAULT);
	ATTRIBUTE(PMXIKSolver, VAR_INT, "Loop Count", loopCount, 20, AM_DEFAULT);
}

void PMXIKSolver::AddNode(PMXIKNode *node)
{
	nodes.Push(node);

	chainLength += node->GetBoneLength();
}

void PMXIKSolver::FixedUpdate(float timeStep)
{
	// Perform IK here
	Vector<Vector3> jointPositions;
	PODVector<float> boneLengths;
	for (int idx = nodes.Size() - 1; idx >= 0; --idx) {
		boneLengths.Push(nodes[idx]->GetBoneLength());
		jointPositions.Push(nodes[idx]->GetNode()->GetWorldPosition());
	}

	Vector3 targetPosition = node_->GetWorldPosition();
	Vector3 rootPosition = jointPositions[0];

	// Check if target position is reachable
	if ((rootPosition - targetPosition).LengthSquared() > chainLength * chainLength) {
		for (int i = 0; i < jointPositions.Size() - 1; ++i) {
			float distance = (targetPosition - jointPositions[i]).Length();
			float ratio = boneLengths[i] / distance;
			jointPositions[i + 1] = (1 - ratio) * jointPositions[i] + ratio * targetPosition;
		}
	}
	else {
		for (int i = 0; i < loopCount; ++i) {
			// Break out if the end node is close to its target
			if ((jointPositions.Back() - targetPosition).LengthSquared() < threshold)
				break;

			// Stage 1: Forward reaching
			// Here we start at the root of the chain and iterate towards the end
			jointPositions.Back() = targetPosition;
			for (int n = jointPositions.Size() - 2; n >= 0; --n) {
				float distance = (jointPositions[n + 1] - jointPositions[n]).Length();
				float ratio = boneLengths[n] / distance;
				jointPositions[n] = (1 - ratio) * jointPositions[n + 1] + ratio * jointPositions[n];
			}

			// Stage 2: Backward reaching
			// Inverse of the first stage
			jointPositions.Front() = rootPosition;
			for (int n = 0; n < jointPositions.Size() - 1; ++n) {
				float distance = (jointPositions[n + 1] - jointPositions[n]).Length();
				float ratio = boneLengths[n] / distance;
				jointPositions[n + 1] = (1 - ratio) * jointPositions[n] + ratio * jointPositions[n + 1];
			}
		}
	}

	jointPositions.Push(targetPosition);
	Quaternion oldRotation = endNode->GetWorldRotation();

	// Apply the new positions
	for (int idx = 0; idx < jointPositions.Size() - 1; ++idx) {
		auto node = nodes[nodes.Size() - idx - 1];
		Vector3 position = node->GetNode()->GetWorldPosition();

		Vector3 currentDirection = node->GetNode()->GetWorldUp().Normalized();
		Vector3 desiredDirection = (jointPositions[idx + 1] - jointPositions[idx]).Normalized();

		Vector3 axis = currentDirection.CrossProduct(desiredDirection);
		float dot = currentDirection.AbsDotProduct(desiredDirection);

		if (fabsf(dot) > 1.0f - threshold || axis.Equals(Vector3::ZERO)) continue;

		axis.Normalize();
		float angle = acosf(dot);
			
		node->GetNode()->Rotate(Quaternion(-angle, axis), TS_WORLD);
	}

	endNode->SetWorldRotation(oldRotation);
}

void PMXIKSolver::PerformInnerIteration(Urho3D::Vector3 &parentPosition, Urho3D::Vector3 &linkPosition, Urho3D::Vector3 &targetPosition, PMXIKNode* node, bool reverse)
{
#if 0
	// Now apply rotation constraints if needed
	if (!Link.Limited || ParentBonePosition.isZero())
		return;

	btVector3 LineDirection = (LinkPosition - ParentBonePosition).normalize();

	btVector3 Origin = LineDirection.dot(TargetPosition - LinkPosition) * LineDirection;
	float DistanceToOrigin = Origin.length();
	Origin += LinkPosition;

	btVector3 LocalXDirection(1, 0, 0);
	btQuaternion YRotation;
	{
		// Calculate the Y axis rotation and rotate the X axis for that rotation
		float YRotationAngle = btAcos(LineDirection.dot(btVector3(0, 1, 0)));
		btVector3 YRotationAxis = LineDirection.cross(btVector3(0, 1, 0));
		YRotation = btQuaternion(YRotationAxis, YRotationAngle);
		LocalXDirection = quatRotate(YRotation, LocalXDirection);
	}

	btVector3 LocalPosition = Origin - LinkPosition;
	float Theta = btAcos(LocalPosition.normalized().dot(LocalXDirection));
	int Quadrant = (int)(Theta / DirectX::XM_PIDIV2);
	float Q1, Q2;

	switch (Quadrant) {
	default:
		Q1 = DistanceToOrigin * tanf(Link.Limits.Upper[0]);
		Q2 = DistanceToOrigin * tanf(Link.Limits.Upper[2]);
		break;
	case 1:
		Q1 = DistanceToOrigin * tanf(Link.Limits.Lower[0]);
		Q2 = DistanceToOrigin * tanf(Link.Limits.Upper[2]);
		break;
	case 2:
		Q1 = DistanceToOrigin * tanf(Link.Limits.Lower[0]);
		Q2 = DistanceToOrigin * tanf(Link.Limits.Lower[2]);
		break;
	case 3:
		Q1 = DistanceToOrigin * tanf(Link.Limits.Upper[0]);
		Q2 = DistanceToOrigin * tanf(Link.Limits.Lower[2]);
		break;
	}

	float X = Q1 * cosf(Theta);
	float Y = Q2 * sinf(Theta);
	LocalPosition.setX(powf(-1.0f, Quadrant) * (fabsf(X) < fabsf(LocalPosition.x()) ? fabsf(X) : fabsf(LocalPosition.x())));
	LocalPosition.setY(0.0f);
	LocalPosition.setZ(powf(-1.0f, Quadrant) * (fabsf(Y) < fabsf(LocalPosition.z()) ? fabsf(Y) : fabsf(LocalPosition.z())));

	auto DesiredPosition = (quatRotate(YRotation, LocalPosition) + Origin).lerp(LinkPosition, BoneLength / DistanceToOrigin);
	btVector3 CurrentDirection = (TargetPosition - LinkPosition).normalized();
	btVector3 DesiredDirection = (DesiredPosition - LinkPosition).normalized();
	btVector3 Axis = CurrentDirection.cross(DesiredDirection);
	float Dot = CurrentDirection.dot(DesiredDirection);
	if (Axis.length2() < 0.000001f) {
		LinkPosition = DesiredPosition;
		return;
	}

	Axis.normalize();
	float Angle = btClamped(btAcos(Dot), Link.Limits.Lower[1], Link.Limits.Upper[1]);

	LinkPosition = quatRotate(btQuaternion(Axis, Angle), CurrentDirection) * BoneLength + ParentBonePosition;
#endif
}

