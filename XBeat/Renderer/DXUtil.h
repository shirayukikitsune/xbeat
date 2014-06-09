#pragma once

// Include all commonly used Direct3D headers here
#include <D3D11.h>
#include <DirectXMath.h>

#define DX_DELETEIF(v) if (v) { v->Release(); v = nullptr; }