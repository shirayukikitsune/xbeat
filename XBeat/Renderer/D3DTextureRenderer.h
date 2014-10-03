#pragma once

#include "DXUtil.h"

namespace Renderer {

class D3DTextureRenderer
{
public:
	D3DTextureRenderer();
	~D3DTextureRenderer();

	bool Initialize(ID3D11Device *device, int width, int height, float farZ, float nearZ);
	void Shutdown();

	void SetRenderTarget(ID3D11DeviceContext *context, ID3D11DepthStencilView *depthStencilView);
	void ClearRenderTarget(ID3D11DeviceContext *context, ID3D11DepthStencilView *depthStencilView, float red, float green, float blue, float alpha);
	ID3D11ShaderResourceView* GetShaderResourceView();
	ID3D11ShaderResourceView* GetDepthStencilView();
	void CopyIntoTexture(ID3D11DeviceContext* context, ID3D11Texture2D **target);

	void GetProjectionMatrix(DirectX::XMMATRIX &projection);
	void GetOrthoMatrix(DirectX::XMMATRIX &ortho);

	int GetTextureWidth();
	int GetTextureHeight();

private:
	ID3D11Texture2D *m_renderTargetTexture;
	ID3D11RenderTargetView *m_renderTargetView;
	ID3D11ShaderResourceView *m_shaderResourceView;
	ID3D11ShaderResourceView *m_depthResourceView;
	ID3D11DepthStencilView *m_depthStencilView;
	ID3D11Texture2D *m_depthStencilBuffer;
	D3D11_VIEWPORT m_viewport;
	DirectX::XMMATRIX m_projection, m_ortho;
	int m_texWidth, m_texHeight;
};

}
