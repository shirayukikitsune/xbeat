#pragma once

#include "PMXDefinitions.h"
#include "PMXLoader.h"

namespace Renderer {
namespace PMX {
class Model;

ATTRIBUTE_ALIGNED16(class) KinematicMotionState : public btMotionState
{
public:
	KinematicMotionState(const btTransform &startTrans, const btTransform &boneTrans, Bone *bone);

	virtual ~KinematicMotionState();

	virtual void getWorldTransform(btTransform &worldTrans) const;

	virtual void setWorldTransform(const btTransform &worldTrans);

	BT_DECLARE_ALIGNED_ALLOCATOR();

private:
	Bone *m_bone;
	btTransform m_transform;
};

ATTRIBUTE_ALIGNED16(class) RigidBody
{
public:
	RigidBody();
	~RigidBody();

	void Initialize(std::shared_ptr<Physics::Environment> physics, PMX::Model *model, Loader::RigidBody* body);
	void Shutdown();

	__forceinline Bone* getAssociatedBone() { return m_bone; }

	BT_DECLARE_ALIGNED_ALLOCATOR();

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

