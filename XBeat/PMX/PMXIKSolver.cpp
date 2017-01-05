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

    URHO3D_ATTRIBUTE("Threshold", float, threshold, 0.000001f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Angle Limit", float, angleLimit, M_PI, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Loop Count", int, loopCount, 20, AM_DEFAULT);
}

void PMXIKSolver::AddNode(PMXIKNode *node)
{
	nodes.push_back(node);
    cachedNodeTransforms.resize(nodes.size());
}

void PMXIKSolver::FixedUpdate(float timeStep)
{
	// Perform IK here
	for (int idx = nodes.size() - 1; idx >= 0; --idx) {
		cachedNodeTransforms[idx] = nodes[idx]->GetNode()->GetWorldTransform();
	}

	Vector3 targetPosition = node_->GetWorldPosition();

    SolveForward(targetPosition);

    SolveBackward(nodes[cachedNodeTransforms.size() - 1]->GetNode()->GetWorldPosition());

    endNode->SetWorldPosition(cachedNodeTransforms[0].Translation());

	// Apply the new positions
	for (unsigned idx = 1; idx < cachedNodeTransforms.size(); ++idx) {
		Vector3 desiredDirection = (cachedNodeTransforms[idx - 1].Translation() - cachedNodeTransforms[idx].Translation()).Normalized();

		auto node = nodes[idx];
		node->GetNode()->SetWorldPosition(cachedNodeTransforms[idx].Translation());
		//node->GetNode()->SetWorldDirection(desiredDirection);
	}
}

void PMXIKSolver::SolveForward(const Vector3 & destination)
{
    cachedNodeTransforms[0].SetTranslation(destination);

    Vector3 pos;

    for (unsigned i = 1; i < cachedNodeTransforms.size(); ++i) {
        pos = cachedNodeTransforms[i - 1].Translation() + (cachedNodeTransforms[i].Translation() - cachedNodeTransforms[i - 1].Translation()).Normalized() * nodes[i]->GetBoneLength();
        cachedNodeTransforms[i].SetTranslation(pos);

        // TODO: limits
    }
}

void PMXIKSolver::SolveBackward(const Urho3D::Vector3 & destination)
{
    cachedNodeTransforms[cachedNodeTransforms.size() - 1].SetTranslation(destination);
    
    Vector3 pos;

    for (int i = cachedNodeTransforms.size() - 2; i >= 0; --i) {
        pos = cachedNodeTransforms[i + 1].Translation() + (cachedNodeTransforms[i].Translation() - cachedNodeTransforms[i + 1].Translation()).Normalized() * nodes[i + 1]->GetBoneLength();
        cachedNodeTransforms[i].SetTranslation(pos);
    }
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
	float quadrant = truncf(theta / M_HALF_PI);
	float q1, q2;

	switch ((int)quadrant)
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

