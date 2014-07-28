#pragma once

#include "PMXDefinitions.h"
#include "PMXLoader.h"
#include "GeometricPrimitive.h"

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

	void Initialize(ID3D11DeviceContext *context, std::shared_ptr<Physics::Environment> physics, PMX::Model *model, Loader::RigidBody* body);
	void Shutdown();

	bool XM_CALLCONV Render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

	__forceinline Bone* getAssociatedBone() { return m_bone; }

	BT_DECLARE_ALIGNED_ALLOCATOR();

	void Update();

	operator btRigidBody*();
	btRigidBody* getBody() { return (btRigidBody*)*this; }

private:
	Bone* m_bone;
	btTransform m_transform, m_inverseTransform;
	Name m_name;
	uint16_t m_groupId;
	uint16_t m_groupMask;
	RigidBodyMode m_mode;
	RigidBodyShape m_shapeType;
	btVector3 m_size;

	std::unique_ptr<DirectX::GeometricPrimitive> m_primitive;
	std::shared_ptr<btCollisionShape> m_shape;
	std::shared_ptr<btRigidBody> m_body;
	std::shared_ptr<btPairCachingGhostObject> m_ghost;
	std::shared_ptr<btMotionState> m_motion;
	std::shared_ptr<KinematicMotionState> m_kinematic;
};
}
}

