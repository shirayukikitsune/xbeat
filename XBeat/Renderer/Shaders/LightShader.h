#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

#include "../DXUtil.h"
#include "VertexTypes.h"

namespace Renderer {
	namespace Shaders {

class Light
{
public:
	Light(void);
	~Light(void);

	struct MatrixBufferType {
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX wvp;
	};

	struct CameraBufferType {
		DirectX::XMFLOAT3A cameraPosition;
	};

	struct LightBufferType {
		DirectX::XMFLOAT4 ambientColor;
		DirectX::XMFLOAT4 diffuseColor;
		DirectX::XMFLOAT3 lightDirection;
		float specularPower;
		DirectX::XMFLOAT4 specularColor;
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
		DirectX::XMFLOAT2 padding;
	};

	struct MaterialInfoType {
		uint32_t materialIndex;
		DirectX::XMFLOAT3 padding;
	};

	bool Initialize(ID3D11Device *device, HWND wnd);
	void Shutdown();
	bool Render(ID3D11DeviceContext *context, int indexCount, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView **textures, int textureCount, DirectX::XMFLOAT3 lightDirection, DirectX::XMFLOAT4 diffuseColor, DirectX::XMFLOAT4 ambientColor, DirectX::XMFLOAT3 cameraPosition, DirectX::XMFLOAT4 specularColor, float specularPower, DXType<ID3D11Buffer> materialBuffer, uint32_t materialIndex);
	void RenderShader(ID3D11DeviceContext *context, int indexCount, ID3D11ShaderResourceView **textures, int textureCount);

	bool SetShaderParameters(ID3D11DeviceContext *context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX wvp, DirectX::XMFLOAT3 lightDirection, DirectX::XMFLOAT4 diffuseColor, DirectX::XMFLOAT4 ambientColor, DirectX::XMFLOAT3 cameraPosition, DirectX::XMFLOAT4 specularColor, float specularPower, DXType<ID3D11Buffer> materialBuffer);
	bool SetMaterialInfo(ID3D11DeviceContext *context, uint32_t materialIndex);

private:
	bool InitializeShader(ID3D11Device *device, HWND wnd, const std::wstring &vsFile, const std::wstring &psFile);
	void ShutdownShader();
	void OutputShaderErrorMessage(ID3D10Blob *errorMessage, HWND wnd, const std::wstring &file);

	ID3D11VertexShader *vertexShader;
	ID3D11PixelShader *pixelShader;
	ID3D11InputLayout *layout;
	ID3D11Buffer *matrixBuffer;
	ID3D11SamplerState *sampleState;
	ID3D11Buffer *cameraBuffer;
	ID3D11Buffer *lightBuffer;
	DXType<ID3D11Buffer> materialInfoBuffer;
};

	}
}