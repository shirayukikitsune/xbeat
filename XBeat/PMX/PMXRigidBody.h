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

	void Initialize(std::shared_ptr<Physics::Environment> physics, PMX::Model *model, Loader::RigidBody* body);
	void InitializeDebug(ID3D11DeviceContext *Context);
	void Shutdown();

	bool XM_CALLCONV Render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

	Bone* getAssociatedBone() { return m_bone; }

	void Update();

	operator btRigidBody*();
	btRigidBody* getBody() { return (btRigidBody*)*this; }

	bool isDynamic() { return m_mode == RigidBodyMode::Dynamic; }

#if defined _M_IX86 && defined _MSC_VER
	void *__cdecl operator new(size_t count) {
		return _aligned_malloc(count, 16);
	}

	void __cdecl operator delete(void *object) {
		_aligned_free(object);
	}
#endif

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

