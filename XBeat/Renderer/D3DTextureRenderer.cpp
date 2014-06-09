#include "D3DTextureRenderer.h"

using Renderer::D3DTextureRenderer;


D3DTextureRenderer::D3DTextureRenderer(void)
{
}


D3DTextureRenderer::~D3DTextureRenderer(void)
{
}


bool D3DTextureRenderer::Initialize(ID3D11Device *device, int width, int height, float farZ, float nearZ)
{
	HRESULT result;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_TEXTURE2D_DESC texDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC depthResourceViewDesc;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;

	m_texWidth = width;
	m_texHeight = height;

	ZeroMemory(&texDesc, sizeof (D3D11_TEXTURE2D_DESC));

	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	result = device->CreateTexture2D(&texDesc, NULL, &m_renderTargetTexture);
	if (FAILED(result))
		return false;

	renderTargetViewDesc.Format = texDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	result = device->CreateRenderTargetView(m_renderTargetTexture, &renderTargetViewDesc, &m_renderTargetView);
	if (FAILED(result))
		return false;

	shaderResourceViewDesc.Format = texDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(m_renderTargetTexture, &shaderResourceViewDesc, &m_shaderResourceView);
	if (FAILED(result))
		return false;

	ZeroMemory(&depthBufferDesc, sizeof (D3D11_TEXTURE2D_DESC));
	depthBufferDesc.Width = width;
	depthBufferDesc.Height = height;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	
	result = device->CreateTexture2D(&depthBufferDesc, NULL, &m_depthStencilBuffer);
	if (FAILED(result))
		return false;

	ZeroMemory(&depthStencilViewDesc, sizeof (D3D11_DEPTH_STENCIL_VIEW_DESC));

	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	result = device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView);
	if (FAILED(result))
		return false;

	depthResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	depthResourceViewDesc.Texture2D.MipLevels = 1;
	depthResourceViewDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(m_depthStencilBuffer, &depthResourceViewDesc, &m_depthResourceView);
	if (FAILED(result))
		return false;

	m_viewport.Width = (float)width;
	m_viewport.Height = (float)height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;

	m_projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, (float)width / (float)height, nearZ, farZ);
	m_ortho = DirectX::XMMatrixOrthographicLH((float)width, (float)height, nearZ, farZ);

	return true;
}

void D3DTextureRenderer::Shutdown()
{
	DX_DELETEIF(m_depthResourceView);
	DX_DELETEIF(m_depthStencilView);
	DX_DELETEIF(m_depthStencilBuffer);
	DX_DELETEIF(m_shaderResourceView);
	DX_DELETEIF(m_renderTargetView);
	DX_DELETEIF(m_renderTargetTexture);
}

void D3DTextureRenderer::SetRenderTarget(ID3D11DeviceContext *context, ID3D11DepthStencilView *depthStencilView)
{
	context->OMSetRenderTargets(1, &m_renderTargetView, depthStencilView);

	context->RSSetViewports(1, &m_viewport);
}

void D3DTextureRenderer::ClearRenderTarget(ID3D11DeviceContext *context, ID3D11DepthStencilView *depthStencilView, float red, float green, float blue, float alpha)
{
	float color[4] = { red, green, blue, alpha };

	context->ClearRenderTargetView(m_renderTargetView, color);
	if (depthStencilView != NULL)
		context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void D3DTextureRenderer::CopyIntoTexture(ID3D11DeviceContext *context, ID3D11Texture2D **texture)
{
	context->CopyResource(*texture, m_renderTargetTexture);
}

ID3D11ShaderResourceView* D3DTextureRenderer::GetShaderResourceView()
{
	return m_shaderResourceView;
}

ID3D11ShaderResourceView* D3DTextureRenderer::GetDepthStencilView()
{
	return m_depthResourceView;
}

void D3DTextureRenderer::GetProjectionMatrix(DirectX::XMMATRIX &projection)
{
	projection = m_projection;
}

void D3DTextureRenderer::GetOrthoMatrix(DirectX::XMMATRIX &ortho)
{
	ortho = m_ortho;
}

int D3DTextureRenderer::GetTextureWidth()
{
	return m_texWidth;
}

int D3DTextureRenderer::GetTextureHeight()
{
	return m_texHeight;
}


