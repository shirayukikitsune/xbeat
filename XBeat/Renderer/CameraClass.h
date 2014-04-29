#pragma once

#include "PMX/PMXDefinitions.h"
#include <DirectXMath.h>

namespace Renderer {

ATTRIBUTE_ALIGNED16(class) CameraClass
{
public:
	CameraClass(void);
	~CameraClass(void);

	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z);
	void SetRotation(btVector3& axis, float angle);
	void SetRotation(btQuaternion& quaternion);

	DirectX::XMVECTOR GetPosition();
	DirectX::XMVECTOR GetRotation();

	void Render();
	void GetViewMatrix(DirectX::XMMATRIX &matrix);

	BT_DECLARE_ALIGNED_ALLOCATOR();

private:
	DirectX::XMVECTOR position, rotation;
	DirectX::XMMATRIX viewMatrix;
};

}
