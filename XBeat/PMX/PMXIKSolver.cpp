#include "PMXIKSolver.h"

#include "PMXIKNode.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/Scene/Node.h>

using namespace Urho3D;

PMXIKSolver::PMXIKSolver(Urho3D::Context *context)
	: Urho3D::LogicComponent(context)
{
	threshold = 0.01f;
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

	ATTRIBUTE("Threshold", float, threshold, 0.000001f, AM_DEFAULT);
	ATTRIBUTE("Angle Limit", float, angleLimit, M_PI, AM_DEFAULT);
	ATTRIBUTE("Loop Count", int, loopCount, 20, AM_DEFAULT);
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
			jointPositions[i + 1] = jointPositions[i].Lerp(targetPosition, ratio);
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
				jointPositions[n] = jointPositions[n + 1].Lerp(jointPositions[n], ratio);
				auto node = nodes[nodes.Size() - 1 - n];
				PerformInnerIteration(node->GetNode()->GetParent()->GetPosition(), jointPositions[n], jointPositions[n + 1], node);
			}

			// Stage 2: Backward reaching
			// Inverse of the first stage
			jointPositions.Front() = rootPosition;
			for (int n = 0; n < jointPositions.Size() - 1; ++n) {
				float distance = (jointPositions[n + 1] - jointPositions[n]).Length();
				float ratio = boneLengths[n] / distance;
				jointPositions[n + 1] = jointPositions[n].Lerp(jointPositions[n + 1], ratio);
				auto node = nodes[nodes.Size() - 1 - n];
				PerformInnerIteration(node->GetNode()->GetParent()->GetPosition(), jointPositions[n], jointPositions[n + 1], node);
			}
		}
	}

	jointPositions.Push(targetPosition);
	Quaternion oldRotation = endNode->GetWorldRotation();

	// Apply the new positions
	for (int idx = 0; idx < jointPositions.Size() - 1; ++idx) {
		Vector3 desiredDirection = (jointPositions[idx + 1] - jointPositions[idx]).Normalized();

		auto node = nodes[nodes.Size() - idx - 1];
		node->GetNode()->SetWorldPosition(jointPositions[idx]);
		node->GetNode()->SetWorldDirection(desiredDirection);
	}

	endNode->SetWorldRotation(oldRotation);
}

void PMXIKSolver::PerformInnerIteration(const Urho3D::Vector3 &parentPosition, Urho3D::Vector3 &linkPosition, Urho3D::Vector3 &targetPosition, PMXIKNode* node)
{
	if (!node->IsLimited())
		return;

	Vector3 parentDirection = (linkPosition - parentPosition).Normalized();
	Vector3 localOrigin = parentDirection.DotProduct(targetPosition - linkPosition) * parentDirection;

	float distanceToOrigin = localOrigin.Length();
	localOrigin += linkPosition;

	Vector3 localXDirection = Vector3::RIGHT;
	Quaternion Yrotation;
	{
		float YrotationAngle = acosf(parentDirection.DotProduct(Vector3::UP));
		Vector3 rotationAxis = parentDirection.CrossProduct(Vector3::UP);
		Yrotation = YrotationAngle < threshold ? Quaternion::IDENTITY : Quaternion(YrotationAngle, rotationAxis);
		localXDirection = Yrotation * localXDirection;
	}

	Vector3 localPosition = targetPosition - localOrigin;
	float theta = acosf(localPosition.Normalized().DotProduct(localXDirection));
	int quadrant = (int)(theta / M_HALF_PI);
	float q1, q2;

	switch (quadrant)
	{
	default:
		q1 = distanceToOrigin * tanf(node->GetUpperLimit().x_);
		q2 = distanceToOrigin * tanf(node->GetUpperLimit().z_);
		break;
	case 1:
		q1 = distanceToOrigin * tanf(node->GetLowerLimit().x_);
		q2 = distanceToOrigin * tanf(node->GetUpperLimit().z_);
		break;
	case 2:
		q1 = distanceToOrigin * tanf(node->GetLowerLimit().x_);
		q2 = distanceToOrigin * tanf(node->GetLowerLimit().z_);
		break;
	case 3:
		q1 = distanceToOrigin * tanf(node->GetUpperLimit().x_);
		q2 = distanceToOrigin * tanf(node->GetLowerLimit().z_);
		break;
	}

	float x = q1 * cosf(theta);
	float y = q2 * sinf(theta);
	localPosition.x_ = powf(-1.0f, quadrant) * (fabsf(y) < fabsf(localPosition.x_) ? fabsf(y) : fabsf(localPosition.x_));
	localPosition.y_ = 0.0f;
	localPosition.z_ = powf(-1.0f, quadrant) * (fabsf(x) < fabsf(localPosition.z_) ? fabsf(x) : fabsf(localPosition.z_));

	Vector3 desiredPosition = (Yrotation * localPosition) + localOrigin;
//	Vector3 currentDirection = (targetPosition - linkPosition).Normalized();
//	Vector3 desiredDirection = (desiredPosition - linkPosition).Normalized();
//	Vector3 axis = currentDirection.CrossProduct(desiredDirection);
//	float dot = currentDirection.DotProduct(desiredDirection);
	targetPosition = desiredPosition;

//	axis.Normalize();
//	float angle = Clamp(acosf(dot), node->GetLowerLimit().y_, node->GetUpperLimit().y_);

//	linkPosition = (Quaternion(angle, axis) * currentDirection) * node->GetBoneLength() + parentPosition;
}

