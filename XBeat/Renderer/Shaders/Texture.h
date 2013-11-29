#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

namespace Renderer {
	namespace Shaders {
class Texture
{
public:
	Texture(void);
	~Texture(void);

	struct MatrixBufferType {
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	};

	bool Initialize(ID3D11Device *device, HWND wnd);
	void Shutdown();
	bool Render(ID3D11DeviceContext *context, int indexCount, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView *texture);

	static void OutputShaderErrorMessage(ID3D10Blob *errorMessage, HWND wnd, const std::wstring &file);
private:
	bool InitializeShader(ID3D11Device *device, HWND wnd, const std::wstring &vsFile, const std::wstring &psFile);
	void ShutdownShader();

	bool SetShaderParameters(ID3D11DeviceContext *context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView *texture);
	void RenderShader(ID3D11DeviceContext *context, int indexCount);

	ID3D11VertexShader *vertexShader;
	ID3D11PixelShader *pixelShader;
	ID3D11InputLayout *layout;
	ID3D11Buffer *matrixBuffer;
	ID3D11SamplerState *sampleState;
};

	}
}