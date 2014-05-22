#pragma once

#include "PMXDefinitions.h"
#include "../Light.h"
#include "../DXUtil.h"
#include <d3dx11effect.h>

namespace Renderer {
namespace PMX {

class PMXShader
{
public:
	struct Limits {
		enum {
			Morphs = 50,
			Bones = 250,
			Lights = 3,
		};
	};

	struct ConstBuffer {
		struct MatrixBuffer {
			DirectX::XMMATRIX world, view, projection;
			DirectX::XMVECTOR eyePosition;
		} matrices;

		struct MaterialBuffer {
			DirectX::XMVECTOR ambient, diffuse, specular;
			DirectX::XMVECTOR mulBaseCoeff, mulSphereCoeff, mulToonCoeff;
			DirectX::XMVECTOR addBaseCoeff, addSphereCoeff, addToonCoeff;

			uint32_t flags;
			float morphWeight;
			int index;
			int padding;
		} material;

		struct LightsBuffer {
			DirectX::XMVECTOR ambient[Limits::Lights], diffuse[Limits::Lights], specular[Limits::Lights];
			DirectX::XMVECTOR direction[Limits::Lights];
		} lights;
	};

	PMXShader();
	~PMXShader();

private:

	enum struct ConstBufferDirtyFlags {
		None = 0x0,
		Matrices = 0x1,
		Material = 0x2,
		Lights = 0x4,
		All = 0xFF
	};
	
	uint32_t m_cbufferDirty;

	__forceinline bool IsDirty(ConstBufferDirtyFlags flags = ConstBufferDirtyFlags::All) {
		return (m_cbufferDirty & (uint32_t)flags) != 0;
	}
	__forceinline void SetDirty(ConstBufferDirtyFlags flag) {
		m_cbufferDirty |= (uint32_t)flag;
	}
	__forceinline void ClearDirty(ConstBufferDirtyFlags flag) {
		m_cbufferDirty &= ~((uint32_t)flag);
	}
};

}
}
