#pragma once

#include "PMXDefinitions.h"
#include "../Shaders/GenericShader.h"
#include <d3dx11effect.h>
#include <array>
#include <map>

namespace Renderer {
namespace PMX {

class PMXShader
	: public Shaders::Generic
{
public:
	struct Limits {
		enum {
			Morphs = 50,
			Bones = 250,
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

	struct VertexType {
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 uv;
		//DirectX::XMVECTOR uvEx[4];
		DirectX::XMUINT4 boneIndices;
		DirectX::XMUINT4 boneWeights;
		UINT materialIndex;
	};

	MaterialBufferType& GetMaterial(int index) { return m_materials[index]; }
	bool UpdateMaterialBuffer(ID3D11DeviceContext *context);

private:
	ID3D11Buffer *m_materialBuffer, *m_tmpMatBuffer;
	ID3D11ShaderResourceView *m_materialSrv;
	ID3D11VertexShader *m_vertexShader;
	ID3D11PixelShader *m_pixelShader;
	std::array<MaterialBufferType, Limits::Materials> m_materials;

protected:
	virtual bool InternalInitializeBuffers(ID3D11Device *device, HWND hwnd);
	virtual bool InternalRender(ID3D11DeviceContext *context, UINT indexCount, UINT offset);
	virtual void InternalShutdown();
	virtual void InternalPrepareRender(ID3D11DeviceContext *context);
};

}
}
