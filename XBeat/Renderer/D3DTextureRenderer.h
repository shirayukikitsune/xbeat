#pragma once

#include "PMX/PMXDefinitions.h"
#include "DXUtil.h"

namespace Renderer {

ATTRIBUTE_ALIGNED16(class) D3DTextureRenderer
{
public:
	D3DTextureRenderer();
	~D3DTextureRenderer();

	bool Initialize(ID3D11Device *device, int width, int height, float farZ, float nearZ);
	void Shutdown();

	void SetRenderTarget(ID3D11DeviceContext *context, ID3D11DepthStencilView *depthStencilView);
	void ClearRenderTarget(ID3D11DeviceContext *context, ID3D11DepthStencilView *depthStencilView, float red, float green, float blue, float alpha);
	DXType<ID3D11ShaderResourceView> GetShaderResourceView();
	DXType<ID3D11ShaderResourceView> GetDepthStencilView();
	void CopyIntoTexture(ID3D11DeviceContext* context, ID3D11Texture2D **target);

	void GetProjectionMatrix(DirectX::XMMATRIX &projection);
	void GetOrthoMatrix(DirectX::XMMATRIX &ortho);

	int GetTextureWidth();
	int GetTextureHeight();

	BT_DECLARE_ALIGNED_ALLOCATOR();

private:
	DXType<ID3D11Texture2D> m_renderTargetTexture;
	DXType<ID3D11RenderTargetView> m_renderTargetView;
	DXType<ID3D11ShaderResourceView> m_shaderResourceView;
	DXType<ID3D11ShaderResourceView> m_depthResourceView;
	DXType<ID3D11DepthStencilView> m_depthStencilView;
	DXType<ID3D11Texture2D> m_depthStencilBuffer;
	D3D11_VIEWPORT m_viewport;
	DirectX::XMMATRIX m_projection, m_ortho;
	int m_texWidth, m_texHeight;
};

}
