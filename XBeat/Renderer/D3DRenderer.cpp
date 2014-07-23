#include "D3DRenderer.h"

#include <vector>
#include <algorithm>

using namespace std;
using Renderer::D3DRenderer;

D3DRenderer::D3DRenderer(void)
{
	swapChain = nullptr;
	device = nullptr;
	deviceContext = nullptr;
	renderTarget = nullptr;
	depthStencilBuffer = nullptr;
	for (auto &i : depthStencilState)
		i = nullptr;
	depthStencilView = nullptr;
	for (auto &i : rasterState)
		i = nullptr;
	for (auto &i : blendState)
		i = nullptr;
}


D3DRenderer::~D3DRenderer(void)
{
}


bool D3DRenderer::FindBestRefreshRate(int width, int height, uint32_t &numerator, uint32_t &denominator)
{
	HRESULT result;
	uint32_t numModes, i;
	size_t strLength;
	IDXGIFactory *factory;
	IDXGIAdapter *adapter;
	IDXGIOutput *adapterOutput;
	std::vector<DXGI_MODE_DESC> displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;

	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(result))
		return false;

	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
		return false;

	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
		return false;

	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
		return false;

	displayModeList.resize(numModes);

	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList.data());
	if (FAILED(result))
		return false;

	for (i = 0; i < numModes; i++) {
		if (displayModeList[i].Width == (UINT)width && displayModeList[i].Height == (UINT)height) {
			numerator = max(displayModeList[i].RefreshRate.Numerator, numerator);
			denominator = min(displayModeList[i].RefreshRate.Denominator, denominator);
		}
	}

	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
		return false;

	gfxMemory = (int)(adapterDesc.DedicatedVideoMemory >> 20);
	error = wcstombs_s(&strLength, gfxDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
		return false;

	displayModeList.clear();

	adapterOutput->Release();
	adapter->Release();
	factory->Release();

	return true;
}

bool D3DRenderer::Initialize(int width, int height, bool vsync, HWND wnd, bool fullscreen, float screenDepth, float screenNear)
{
	HRESULT result;
	uint32_t numerator, denominator;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	ID3D11Texture2D *backBuffer;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_BLEND_DESC blendStateDesc;
	D3D11_RASTERIZER_DESC rasterDesc;
	float fov, aspect;

	vsyncEnabled = vsync;

	if (!FindBestRefreshRate(width, height, numerator, denominator))
		return false;

	ZeroMemory(&swapChainDesc, sizeof swapChainDesc);
	swapChainDesc.BufferCount = 1;

	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;

	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (vsyncEnabled) {
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else {
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the back buffer usage
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set our render target as our window
	swapChainDesc.OutputWindow = wnd;

	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	swapChainDesc.Windowed = !fullscreen;

	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard back buffer after presenting
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	swapChainDesc.Flags = 0;

	UINT deviceFlags = 0;
#ifdef _DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;
#endif
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, featureLevels, sizeof(featureLevels) / sizeof(featureLevels[0]), D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, NULL, &deviceContext);
	if (FAILED(result))
		return false;

	result = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	if (FAILED(result))
		return false;

	result = device->CreateRenderTargetView(backBuffer, NULL, &renderTarget);
	if (FAILED(result))
		return false;

	backBuffer->Release();

	ZeroMemory(&depthBufferDesc, sizeof depthBufferDesc);

	depthBufferDesc.Width = width;
	depthBufferDesc.Height = height;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	result = device->CreateTexture2D(&depthBufferDesc, NULL, &depthStencilBuffer);
	if (FAILED(result))
		return false;

	ZeroMemory(&depthStencilDesc, sizeof depthStencilDesc);

	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	result = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState[2]);
	if (FAILED(result))
		return false;

	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	result = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState[0]);
	if (FAILED(result))
		return false;

	depthStencilDesc.DepthEnable = false;

	result = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState[3]);
	if (FAILED(result))
		return false;

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	result = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState[1]);
	if (FAILED(result))
		return false;

	deviceContext->OMSetDepthStencilState(depthStencilState[0], 1);

	// Initailze the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	result = device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &depthStencilView);
	if (FAILED(result))
		return false;

	deviceContext->OMSetRenderTargets(1, &renderTarget, depthStencilView);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	srvd.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MipLevels = 1;
	srvd.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(depthStencilBuffer, &srvd, &depthResourceView);
	if (FAILED(result))
		return false;

	// Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.AntialiasedLineEnable = true;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	result = device->CreateRasterizerState(&rasterDesc, &rasterState[0]);
	if (FAILED(result))
		return false;
	deviceContext->RSSetState(rasterState[0]);

	// Create a no-cull state
	rasterDesc.CullMode = D3D11_CULL_NONE;
	result = device->CreateRasterizerState(&rasterDesc, &rasterState[1]);
	if (FAILED(result))
		return false;

	// Create a no-cull state with wireframe
	rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	result = device->CreateRasterizerState(&rasterDesc, &rasterState[2]);
	if (FAILED(result))
		return false;

	ZeroMemory(&blendStateDesc, sizeof (D3D11_BLEND_DESC));

	blendStateDesc.AlphaToCoverageEnable = TRUE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;

	result = device->CreateBlendState(&blendStateDesc, &blendState[0]);
	if (FAILED(result))
		return false;

	blendStateDesc.RenderTarget[0].BlendEnable = FALSE;

	result = device->CreateBlendState(&blendStateDesc, &blendState[1]);
	if (FAILED(result))
		return false;

	EnableAlphaBlending();
		
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	deviceContext->RSSetViewports(1, &viewport);

	fov = DirectX::XM_PIDIV4;
	aspect = (float)width / (float)height;

	projection = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, screenNear, screenDepth);
	worldMatrix = DirectX::XMMatrixIdentity();
	orthoMatrix = DirectX::XMMatrixOrthographicLH((float)width, (float)height, screenNear, screenDepth);

	return true;
}

void D3DRenderer::Shutdown()
{
	if (swapChain)
		swapChain->SetFullscreenState(false, NULL);

	for (auto &state : blendState) {
		if (state != nullptr)
			state->Release();
		state = nullptr;
	}

	for (auto &state : rasterState) {
		if (state != nullptr)
			state->Release();

		state = nullptr;
	}

	if (depthResourceView)
		depthResourceView = nullptr;

	if (depthStencilView) {
		depthStencilView->Release();
		depthStencilView = nullptr;
	}

	for (auto &state : depthStencilState) {
		if (state != nullptr)
			state->Release();
		state = nullptr;
	}

	if (depthStencilBuffer) {
		depthStencilBuffer->Release();
		depthStencilBuffer = nullptr;
	}

	if (renderTarget) {
		renderTarget->Release();
		renderTarget = nullptr;
	}

	if (deviceContext) {
		deviceContext->Release();
		deviceContext = nullptr;
	}

	if (device) {
		device->Release();
		device = nullptr;
	}

	if (swapChain) {
		swapChain->Release();
		swapChain = nullptr;
	}
}

void D3DRenderer::BeginScene(float R, float G, float B, float A)
{
	float color[4] = { R, G, B, A };

	deviceContext->ClearRenderTargetView(renderTarget, color);
	deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void D3DRenderer::EndScene()
{
	swapChain->Present((vsyncEnabled ? 1 : 0), 0);
}

ID3D11Device* D3DRenderer::GetDevice()
{
	return device;
}

ID3D11DeviceContext* D3DRenderer::GetDeviceContext()
{
	return deviceContext;
}

IDXGISwapChain *D3DRenderer::GetSwapChain()
{
	return swapChain;
}

ID3D11RasterizerState* D3DRenderer::GetRasterState(int idx)
{
	return rasterState[idx];
}

void D3DRenderer::SetDepthAlways()
{
	deviceContext->OMSetDepthStencilState(depthStencilState[3], 1);
}

void D3DRenderer::SetDepthLessEqual()
{
	deviceContext->OMSetDepthStencilState(depthStencilState[2], 1);
}

void D3DRenderer::Begin2D()
{
	deviceContext->OMSetDepthStencilState(depthStencilState[1], 1);
}

void D3DRenderer::End2D()
{
	deviceContext->OMSetDepthStencilState(depthStencilState[0], 1);
}

void D3DRenderer::EnableAlphaBlending()
{
	static const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	deviceContext->OMSetBlendState(blendState[0], blendFactor, 0xFFFFFFFFU);
}

void D3DRenderer::DisableAlphaBlending()
{
	static const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	deviceContext->OMSetBlendState(blendState[1], blendFactor, 0xFFFFFFFFU);
}

void D3DRenderer::GetProjectionMatrix(DirectX::XMMATRIX &matrix)
{
	matrix = projection;
}

void D3DRenderer::GetWorldMatrix(DirectX::XMMATRIX &matrix)
{
	matrix = worldMatrix;
}

void D3DRenderer::GetOrthoMatrix(DirectX::XMMATRIX &matrix)
{
	matrix = orthoMatrix;
}

void D3DRenderer::GetVideoCardInfo(char *cardName, int &memory)
{
	strcpy_s(cardName, 128, gfxDescription);
	memory = gfxMemory;
}

ID3D11DepthStencilView* D3DRenderer::GetDepthStencilView()
{
	return depthStencilView;
}

ID3D11Texture2D *D3DRenderer::GetDepthStencilTexture()
{
	return depthStencilBuffer;
}

ID3D11ShaderResourceView *D3DRenderer::GetDepthResourceView()
{
	return depthResourceView;
}

void D3DRenderer::SetBackBufferRenderTarget()
{
	deviceContext->OMSetRenderTargets(1, &renderTarget, depthStencilView);
}

void D3DRenderer::ResetViewport()
{
	deviceContext->RSSetViewports(1, &viewport);
}
