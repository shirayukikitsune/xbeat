#pragma once

#include "PMX/PMXDefinitions.h"
#include "Entity.h"
#include <DirectXMath.h>

namespace Renderer {

ATTRIBUTE_ALIGNED16(class) Camera
	: public Entity
{
public:
	Camera();

	void XM_CALLCONV SetRotation(float x, float y, float z);
	void XM_CALLCONV SetRotation(btVector3& axis, float angle);
	void XM_CALLCONV SetRotation(btQuaternion& quaternion);

	DirectX::XMVECTOR GetRotation();

	virtual bool Update(float msec);

	void XM_CALLCONV GetViewMatrix(DirectX::XMMATRIX &matrix);

	BT_DECLARE_ALIGNED_ALLOCATOR();

private:
	DirectX::XMVECTOR m_rotation;
	DirectX::XMMATRIX m_viewMatrix;
};

}
