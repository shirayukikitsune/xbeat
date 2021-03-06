#pragma once

#include "PMXDefinitions.h"
#include "../Renderer/Shaders/GenericShader.h"
#include <d3dx11effect.h>
#include <array>
#include <map>

namespace PMX {

class PMXShader
	: public Renderer::Shaders::Generic
{
public:
	struct Limits {
		enum {
			Morphs = 50,
			Bones = 1000,
			Materials = 100,
		};
	};

	struct MaterialBufferType {
		DirectX::XMFLOAT4 ambientColor;
		DirectX::XMFLOAT4 diffuseColor;
		DirectX::XMFLOAT4 specularColor;
		DirectX::XMFLOAT4 mulBaseCoefficient;
		DirectX::XMFLOAT4 mulSphereCoefficient;
		DirectX::XMFLOAT4 mulToonCoefficient;
		DirectX::XMFLOAT4 addBaseCoefficient;
		DirectX::XMFLOAT4 addSphereCoefficient;
		DirectX::XMFLOAT4 addToonCoefficient;

		uint32_t flags;
		float morphWeight;
		int index;
		int padding;
	};

	struct BoneBufferType {
		DirectX::XMMATRIX transform;
		DirectX::XMVECTOR position;
	};

	struct VertexType {
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 uv;
		DirectX::XMUINT4 boneIndices;
		DirectX::XMFLOAT4 boneWeights;
		UINT materialIndex;
	};

	MaterialBufferType& GetMaterial(int index) { return m_materials[index]; }
	bool UpdateMaterialBuffer(ID3D11DeviceContext *context);

	BoneBufferType& GetBone(int index) { return m_bones[index]; }
	bool UpdateBoneBuffer(ID3D11DeviceContext *context);

#if defined _M_IX86 && defined _MSC_VER
	void *__cdecl operator new(size_t count) {
		return _aligned_malloc(count, 16);
	}

	void __cdecl operator delete(void *object) {
		_aligned_free(object);
	}
#endif

private:
	ID3D11Buffer *m_materialBuffer, *m_tmpMatBuffer;
	ID3D11ShaderResourceView *m_materialSrv;
	ID3D11Buffer *m_bonesBuffer, *m_tmpBoneBuffer;
	ID3D11ShaderResourceView *m_bonesSrv;
	ID3D11VertexShader *m_vertexShader;
	ID3D11PixelShader *m_pixelShader;
	std::array<MaterialBufferType, Limits::Materials> m_materials;
	std::array<BoneBufferType, Limits::Bones> m_bones;

protected:
	virtual bool InternalInitializeBuffers(ID3D11Device *device, HWND hwnd);
	virtual bool InternalRender(ID3D11DeviceContext *context, UINT indexCount, UINT offset);
	virtual void InternalShutdown();
	virtual void InternalPrepareRender(ID3D11DeviceContext *context);
};

}
