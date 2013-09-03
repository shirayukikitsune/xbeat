#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <D3DX11async.h>
#include <string>

namespace Renderer {
	namespace Shaders {

class Light
{
public:
	Light(void);
	~Light(void);

	struct MatrixBufferType {
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
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
		uint32_t flags;
		DirectX::XMFLOAT4 ambientColor;
		DirectX::XMFLOAT4 diffuseColor;
		DirectX::XMFLOAT4 specularColor;
		DirectX::XMFLOAT3 padding;
	};

	bool Initialize(ID3D11Device *device, HWND wnd);
	void Shutdown();
	bool Render(ID3D11DeviceContext *context, int indexCount, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView **textures, int textureCount, DirectX::XMFLOAT3 lightDirection, DirectX::XMFLOAT4 diffuseColor, DirectX::XMFLOAT4 ambientColor, DirectX::XMFLOAT3 cameraPosition, DirectX::XMFLOAT4 specularColor, float specularPower, MaterialBufferType &materialInfo);

private:
	bool InitializeShader(ID3D11Device *device, HWND wnd, const std::wstring &vsFile, const std::wstring &psFile);
	void ShutdownShader();
	void OutputShaderErrorMessage(ID3D10Blob *errorMessage, HWND wnd, const std::wstring &file);

	bool SetShaderParameters(ID3D11DeviceContext *context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView **textures, int textureCount, DirectX::XMFLOAT3 lightDirection, DirectX::XMFLOAT4 diffuseColor, DirectX::XMFLOAT4 ambientColor, DirectX::XMFLOAT3 cameraPosition, DirectX::XMFLOAT4 specularColor, float specularPower, MaterialBufferType &materialInfo);
	void RenderShader(ID3D11DeviceContext *context, int indexCount);

	ID3D11VertexShader *vertexShader;
	ID3D11PixelShader *pixelShader;
	ID3D11InputLayout *layout;
	ID3D11Buffer *matrixBuffer;
	ID3D11SamplerState *sampleState;
	ID3D11Buffer *cameraBuffer;
	ID3D11Buffer *lightBuffer;
	ID3D11Buffer *materialBuffer;
};

	}
}