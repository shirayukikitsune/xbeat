#pragma once

#include "PMXDefinitions.h"
#include "PMXLoader.h"

namespace Renderer {
namespace PMX {
class Model;

PMX_ALIGN class KinematicMotionState : public btMotionState
{
public:
	KinematicMotionState(const btTransform &startTrans, const btTransform &boneTrans, Bone *bone);

	virtual ~KinematicMotionState();

	virtual void getWorldTransform(btTransform &worldTrans) const;

	virtual void setWorldTransform(const btTransform &worldTrans);

	PMX_ALIGNMENT_OPERATORS

private:
	Bone *m_bone;
	btTransform m_transform;
};

PMX_ALIGN class RigidBody
{
public:
	RigidBody();
	~RigidBody();

	void Initialize(std::shared_ptr<Physics::Environment> physics, PMX::Model *model, Loader::RigidBody* body);
	void Shutdown();

	__forceinline Bone* getAssociatedBone() { return m_bone; }

	PMX_ALIGNMENT_OPERATORS

private:
	Bone* m_bone;
	btTransform m_transform;
	Name m_name;
	uint16_t m_groupId;
	uint16_t m_groupMask;

	std::shared_ptr<btCollisionShape> m_shape;
	std::shared_ptr<btRigidBody> m_body;
	std::shared_ptr<btPairCachingGhostObject> m_ghost;
	std::shared_ptr<btMotionState> m_motion;
};
}
}

