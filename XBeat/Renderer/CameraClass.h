#pragma once

#include "PMX/PMXDefinitions.h"
#include <DirectXMath.h>

namespace Renderer {

PMX_ALIGN class CameraClass
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

	PMX_ALIGNMENT_OPERATORS

private:
	DirectX::XMVECTOR position, rotation;
	DirectX::XMMATRIX viewMatrix;
};

}
