#pragma once

#include "DXUtil.h"

namespace Renderer {

class OrthoWindowClass
{
public:
	OrthoWindowClass();
	~OrthoWindowClass();

	bool Initialize(DXType<ID3D11Device> device, int width, int height);
	void Shutdown();
	void Render(DXType<ID3D11DeviceContext> context);

	int GetIndexCount();

private:
	struct VertexType
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 texture;
	};

	bool InitializeBuffers(DXType<ID3D11Device> device, int width, int height);
	void ShutdownBuffers();
	void RenderBuffers(DXType<ID3D11DeviceContext> context);

	DXType<ID3D11Buffer> m_vertexBuffer;
	DXType<ID3D11Buffer> m_indexBuffer;
	int m_vertexCount;
	int m_indexCount;
};

}
