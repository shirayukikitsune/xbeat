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
	bool Render(ID3D11DeviceContext *context, int indexCount, ID3D11ShaderResourceView *texture);
private:
	bool InitializeShader(ID3D11Device *device, HWND wnd, const std::wstring &vsFile, const std::wstring &psFile);
	void ShutdownShader();

	bool SetShaderParameters(ID3D11DeviceContext *context, ID3D11ShaderResourceView *texture);
	void RenderShader(ID3D11DeviceContext *context, int indexCount);

	ID3D11VertexShader *vertexShader;
	ID3D11PixelShader *pixelShader;
	ID3D11InputLayout *layout;
	ID3D11SamplerState *sampleState;
};

	}
}