#pragma once

#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "DXUtil.h"

#include <memory>

namespace Renderer {

class D3DRenderer
{
public:
	D3DRenderer();
	~D3DRenderer();

	bool Initialize(int width, int height, bool vsync, HWND wnd, bool fullScreen, float depth, float near);
	void Shutdown();

	void BeginScene(float R, float G, float B, float A);
	void EndScene();

	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetDeviceContext();
	IDXGISwapChain *GetSwapChain();

	ID3D11RasterizerState *GetRasterState(int idx);

	void GetOrthoMatrix(DirectX::XMMATRIX &matrix);

	void GetVideoCardInfo(char *name, int &memory);

	void SetDepthAlways();
	void SetDepthLessEqual();
	void Begin2D();
	void End2D();

	void EnableAlphaBlending();
	void DisableAlphaBlending();

	ID3D11DepthStencilView *GetDepthStencilView();
	ID3D11Texture2D *GetDepthStencilTexture();
	void SetBackBufferRenderTarget();

	void ResetViewport();

	ID3D11ShaderResourceView* GetDepthResourceView();
	ID3D11RenderTargetView* renderTarget;

private:
	bool FindBestRefreshRate(int width, int height, uint32_t &numerator, uint32_t &denominator);

	bool vsyncEnabled;
	int gfxMemory;
	char gfxDescription[128];
	IDXGISwapChain *swapChain;
	ID3D11Device *device;
	ID3D11DeviceContext *deviceContext;
	ID3D11Texture2D *depthStencilBuffer;
	ID3D11DepthStencilState *depthStencilState[4];
	ID3D11DepthStencilView *depthStencilView;
	ID3D11ShaderResourceView *depthResourceView;
	ID3D11BlendState *blendState[2];
	ID3D11RasterizerState *rasterState[3];
	D3D11_VIEWPORT viewport;
	DirectX::XMMATRIX orthoMatrix;
};

}
