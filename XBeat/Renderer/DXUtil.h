#pragma once

// Include all commonly used Direct3D headers here
#include <D3D11.h>
#include <DirectXMath.h>
#include <utility>
#include "LinearMath/btTransform.h" // Convert XMTRANSFORM to/from btTransform

#define DX_DELETEIF(v) if (v) { v->Release(); v = nullptr; }

namespace DirectX
{
	inline static btVector3 XMFloat3ToBtVector3(const XMFLOAT3 &in) {
		return btVector3(in.x, in.y, in.z);
	}
}