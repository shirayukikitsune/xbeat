#pragma once

#include <DirectXMath.h>

namespace Renderer {

class ViewFrustum
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

#if defined _M_IX86 && defined _MSC_VER
	void *__cdecl operator new(size_t count) {
		return _aligned_malloc(count, 16);
	}

	void __cdecl operator delete(void *object) {
		_aligned_free(object);
	}
#endif

private:
	DirectX::XMVECTOR planes[6];
};

}
