#include "OrthoWindowClass.h"

#include <memory>

using Renderer::OrthoWindowClass;
using Renderer::DXType;

OrthoWindowClass::OrthoWindowClass(void)
{
	m_vertexCount = 0;
	m_indexCount = 0;
}


OrthoWindowClass::~OrthoWindowClass(void)
{
}

bool OrthoWindowClass::Initialize(DXType<ID3D11Device> device, int width, int height)
{
	if (!InitializeBuffers(device, width, height))
		return false;

	return true;
}

void OrthoWindowClass::Shutdown()
{
	ShutdownBuffers();
}

void OrthoWindowClass::Render(DXType<ID3D11DeviceContext> context)
{
	RenderBuffers(context);
}

int OrthoWindowClass::GetIndexCount()
{
	return m_indexCount;
}

bool OrthoWindowClass::InitializeBuffers(DXType<ID3D11Device> device, int width, int height)
{
	float left, right, top, bottom;
	std::unique_ptr<VertexType> vertices;
	std::unique_ptr<uint32_t> indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int i;

	left = -((float)width / 2.0f);
	right = left + width;
	top = (float)height / 2.0f;
	bottom = top - (float)height;

	m_vertexCount = 6;
	m_indexCount = 6;

	vertices.reset(new VertexType[m_vertexCount]);
	if (!vertices) return false;

	indices.reset(new uint32_t[m_indexCount]);
	if (!indices) return false;

	auto v = vertices.get();
	v[0].position = DirectX::XMFLOAT3(left, top, 0.0f);
	v[0].texture = DirectX::XMFLOAT2(0.0f, 0.0f);

	v[1].position = DirectX::XMFLOAT3(right, bottom, 0.0f);
	v[1].texture = DirectX::XMFLOAT2(1.0f, 1.0f);

	v[2].position = DirectX::XMFLOAT3(left, bottom, 0.0f);
	v[2].texture = DirectX::XMFLOAT2(0.0f, 1.0f);

	v[3].position = DirectX::XMFLOAT3(left, top, 0.0f);
	v[3].texture = DirectX::XMFLOAT2(0.0f, 0.0f);

	v[4].position = DirectX::XMFLOAT3(right, top, 0.0f);
	v[4].texture = DirectX::XMFLOAT2(1.0f, 0.0f);

	v[5].position = DirectX::XMFLOAT3(right, bottom, 0.0f);
	v[5].texture = DirectX::XMFLOAT2(1.0f, 1.0f);

	for (i = 0; i < m_vertexCount; i++) {
		indices.get()[i] = i;
	}

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof (VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = vertices.get();
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
		return false;

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof (uint32_t) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = indices.get();
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
		return false;

	vertices.reset();
	indices.reset();

	return true;
}

void OrthoWindowClass::ShutdownBuffers()
{
	if (m_indexBuffer) {
		m_indexBuffer->Release();
		m_indexBuffer = nullptr;
	}

	if (m_vertexBuffer) {
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}
}

void OrthoWindowClass::RenderBuffers(DXType<ID3D11DeviceContext> context)
{
	uint32_t stride = sizeof (VertexType), offset = 0;

	context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
