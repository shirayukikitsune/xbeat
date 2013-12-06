#pragma once

#include "PMX/PMXDefinitions.h"
#include <DirectXMath.h>

namespace Renderer {

PMX_ALIGN class ViewFrustum
{
public:
	ViewFrustum(void);
	~ViewFrustum(void);

	void Construct(float depth, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX view);

	bool IsPointInside(DirectX::XMFLOAT3 point);
	bool IsPointInside(float x, float y, float z);

	bool IsCubeInside(DirectX::XMFLOAT3 center, float radius);
	bool IsCubeInside(float x, float y, float z, float radius);

	bool IsSphereInside(DirectX::XMFLOAT3 center, float radius);
	bool IsSphereInside(float x, float y, float z, float radius);

	bool IsBoxInside(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 radius);
	bool IsBoxInside(float x, float y, float z, float rx, float ry, float rz);

	PMX_ALIGNMENT_OPERATORS

private:
	DirectX::XMVECTOR planes[6];
};

}
