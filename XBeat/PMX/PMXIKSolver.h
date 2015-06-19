#pragma once

#include <LogicComponent.h>
#include <Vector.h>

class PMXIKNode;

class PMXIKSolver
	: public Urho3D::LogicComponent
{
public:
	PMXIKSolver(Urho3D::Context *context);
	~PMXIKSolver();
	/// Register object factory.
	static void RegisterObject(Urho3D::Context* context);

	virtual void FixedUpdate(float timeStep);

	void AddNode(PMXIKNode *node);

	void SetThreshold(float value) { threshold = value; }
	void SetAngleLimit(float limit) { angleLimit = limit; }
	void SetLoopCount(int count) { loopCount = count; }
	void SetEndNode(Urho3D::Node *node) { endNode = node; }

	float GetThreshold() const { return threshold; }
	float GetAngleLimit() const { return angleLimit; }
	float GetChainLength() const { return chainLength; }
	int GetLoopCount() const { return loopCount; }
	Urho3D::Node* GetEndNode() const { return endNode; }

private:
	void PerformInnerIteration(Urho3D::Vector3 &parentPosition, Urho3D::Vector3 &linkPosition, Urho3D::Vector3 &targetPosition, PMXIKNode* link, bool reverse);

	float threshold;
	float angleLimit;
	float chainLength;
	int loopCount;
	Urho3D::Node *endNode;
	Urho3D::Vector<PMXIKNode*> nodes;
};

