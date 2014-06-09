#include "LightShader.h"
#include <fstream>

using namespace Renderer::Shaders;

bool Light::UpdateMaterialBuffer(MaterialBufferType material, ID3D11DeviceContext *context)
{
	D3D11_MAPPED_SUBRESOURCE map;
	HRESULT hr;

	hr = context->Map(m_materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (FAILED(hr))
		return false;

	memcpy(map.pData, &material, sizeof(MaterialBufferType));

	context->Unmap(m_materialBuffer, 0);

	return true;
}

bool Light::InternalInitializeBuffers(ID3D11Device *device, HWND wnd)
{
	HRESULT result;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[] = {
			{ "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
	D3D11_BUFFER_DESC buffDesc;
	SIZE_T vsbufsize, psbufsize;

	char *vsbuffer = getFileContents(L"./Data/Shaders/LightVertex.cso", vsbufsize);
	result = device->CreateVertexShader(vsbuffer, vsbufsize, NULL, &m_vertexShader);
	if (FAILED(result)) {
		delete[] vsbuffer;
		return false;
	}

	char *psbuffer = getFileContents(L"./Data/Shaders/LightPixel.cso", psbufsize);
	result = device->CreatePixelShader(psbuffer, psbufsize, NULL, &m_pixelShader);
	delete[] psbuffer;
	if (FAILED(result)) {
		delete[] vsbuffer;
		return false;
	}

	result = device->CreateInputLayout(polygonLayout, numElements, vsbuffer, vsbufsize, &m_layout);
	delete[] vsbuffer;
	if (FAILED(result)) {
		return false;
	}

	vsbuffer = nullptr;

	buffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffDesc.ByteWidth = sizeof(MaterialBufferType);
	buffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffDesc.MiscFlags = 0;
	buffDesc.StructureByteStride = 0;
	buffDesc.Usage = D3D11_USAGE_DYNAMIC;

	result = device->CreateBuffer(&buffDesc, NULL, &m_materialBuffer);
	if (FAILED(result))
		return false;

	return true;
}

void Light::InternalPrepareRender(ID3D11DeviceContext *context)
{
	context->IASetInputLayout(m_layout);

	context->VSSetShader(m_vertexShader, NULL, 0);
	context->PSSetShader(m_pixelShader, NULL, 0);

	context->PSSetConstantBuffers(1, 1, &m_materialBuffer);
}

bool Light::InternalRender(ID3D11DeviceContext *context, UINT indexCount, UINT offset)
{
	context->DrawIndexed(indexCount, offset, offset);

	return true;
}

void Light::InternalShutdown()
{
	if (m_materialBuffer) {
		m_materialBuffer->Release();
		m_materialBuffer = nullptr;
	}

	if (m_layout) {
		m_layout->Release();
		m_layout = nullptr;
	}

	if (m_pixelShader) {
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}

	if (m_vertexShader) {
		m_vertexShader->Release();
		m_vertexShader = nullptr;
	}
}
