#pragma once

#include "PMX/PMXDefinitions.h"
#include "Entity.h"
#include <DirectXMath.h>

namespace Renderer {

class Camera
	: public Entity
{
public:
	Camera();

	void XM_CALLCONV SetRotation(float x, float y, float z);
	void XM_CALLCONV SetRotation(DirectX::XMFLOAT3& axis, float angle);
	void XM_CALLCONV SetRotation(DirectX::FXMVECTOR quaternion);

	DirectX::XMVECTOR GetRotation();

	virtual bool Update(float msec);

	void XM_CALLCONV GetViewMatrix(DirectX::XMMATRIX &matrix);

private:
	DirectX::XMVECTOR m_rotation;
	DirectX::XMMATRIX m_viewMatrix;
};

}
