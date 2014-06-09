#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

#include "GenericShader.h"
#include "../DXUtil.h"
#include "VertexTypes.h"

namespace Renderer {
	namespace Shaders {

class Light
	: public Generic
{
public:
	struct VertexInput {
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 uv;
	};

	struct MaterialBufferType {
		DirectX::XMFLOAT4 ambientColor;
		DirectX::XMFLOAT4 diffuseColor;
		DirectX::XMFLOAT4 specularColor;
	};

	bool UpdateMaterialBuffer(MaterialBufferType material, ID3D11DeviceContext *context);

private:
	ID3D11VertexShader *m_vertexShader;
	ID3D11PixelShader *m_pixelShader;
	ID3D11Buffer *m_materialBuffer;

protected:
	virtual bool InternalInitializeBuffers(ID3D11Device *device, HWND hwnd);
	virtual void InternalPrepareRender(ID3D11DeviceContext *context);
	virtual bool InternalRender(ID3D11DeviceContext *context, UINT indexCount, UINT offset);
	virtual void InternalShutdown();
};

	}
}