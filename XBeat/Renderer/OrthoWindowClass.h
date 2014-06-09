#pragma once

#include "DXUtil.h"

namespace Renderer {

class OrthoWindowClass
{
public:
	OrthoWindowClass();
	~OrthoWindowClass();

	bool Initialize(ID3D11Device *device, int width, int height);
	void Shutdown();
	void Render(ID3D11DeviceContext *context);

	int GetIndexCount();

private:
	struct VertexType
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 texture;
	};

	bool InitializeBuffers(ID3D11Device *device, int width, int height);
	void ShutdownBuffers();
	void RenderBuffers(ID3D11DeviceContext *context);

	ID3D11Buffer *m_vertexBuffer;
	ID3D11Buffer *m_indexBuffer;
	int m_vertexCount;
	int m_indexCount;
};

}
