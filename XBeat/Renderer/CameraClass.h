#pragma once

#include <DirectXMath.h>

namespace Renderer {

class CameraClass
{
public:
	CameraClass(void);
	~CameraClass(void);

	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z);

	DirectX::XMVECTOR GetPosition();
	DirectX::XMVECTOR GetRotation();

	void Render();
	void GetViewMatrix(DirectX::XMMATRIX &matrix);

private:
	DirectX::XMVECTOR position, rotation;
	DirectX::XMMATRIX viewMatrix;
};

}
