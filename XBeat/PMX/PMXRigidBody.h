#pragma once

#include "PMXDefinitions.h"
#include "PMXLoader.h"
#include "GeometricPrimitive.h"

namespace Physics { class PMXMotionState; }

namespace PMX {
class Model;

class RigidBody
{
public:
	RigidBody();
	~RigidBody();

	const Name& GetName() const { return m_name; }

	void Initialize(ID3D11DeviceContext *context, std::shared_ptr<Physics::Environment> physics, PMX::Model *model, Loader::RigidBody* body);
	void Shutdown();

	bool XM_CALLCONV Render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

	Bone* getAssociatedBone() { return m_bone; }

	void Update();

	operator btRigidBody*();
	btRigidBody* getBody() { return (btRigidBody*)*this; }

private:
	Bone* m_bone;
	btTransform m_transform, m_inverse;
	Name m_name;
	uint16_t m_groupId;
	uint16_t m_groupMask;
	RigidBodyMode m_mode;
	RigidBodyShape m_shapeType;
	btVector3 m_size;
	DirectX::XMVECTOR m_color;

	std::unique_ptr<DirectX::GeometricPrimitive> m_primitive;
	std::unique_ptr<btCollisionShape> m_shape;
	std::shared_ptr<btRigidBody> m_body;
	std::unique_ptr<btMotionState> m_motion;
	std::unique_ptr<Physics::PMXMotionState> m_kinematic;
};
}

