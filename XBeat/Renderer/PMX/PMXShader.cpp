#include "PMXShader.h"
#include <d3dcompiler.h>
#include "../Entity.h"

using namespace DirectX;
using namespace Renderer;
using Renderer::PMX::PMXShader;

bool PMXShader::UpdateMaterialBuffer(ID3D11DeviceContext *context)
{
	context->UpdateSubresource(m_materialBuffer, 0, NULL, m_materials.data(), sizeof(MaterialBufferType), m_materials.size());

	return true;
}

bool PMXShader::InternalInitializeBuffers(ID3D11Device *device, HWND hwnd)
{
	D3D11_BUFFER_DESC buffDesc;
	D3D11_SUBRESOURCE_DATA data;
	HRESULT result;

	D3D11_INPUT_ELEMENT_DESC polygonLayout[] = {
			{ "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			/*{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },*/
			{ "BONES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BONES", 1, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "MATERIAL", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
	SIZE_T vsbufsize, psbufsize;

	char *vsbuffer = getFileContents(L"./Data/Shaders/PMX/PMXVertex.cso", vsbufsize);
	result = device->CreateVertexShader(vsbuffer, vsbufsize, NULL, &m_vertexShader);
	if (FAILED(result)) {
		delete[] vsbuffer;
		return false;
	}

	char *psbuffer = getFileContents(L"./Data/Shaders/PMX/PMXPixel.cso", psbufsize);
	result = device->CreatePixelShader(psbuffer, psbufsize, NULL, &m_pixelShader);
	delete[] psbuffer;
	if (FAILED(result)) {
		return false;
	}

	result = device->CreateInputLayout(polygonLayout, numElements, vsbuffer, vsbufsize, &m_layout);
	delete[] vsbuffer;
	if (FAILED(result))
		return false;

	buffDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	buffDesc.ByteWidth = sizeof(MaterialBufferType) * Limits::Materials;
	buffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	buffDesc.StructureByteStride = sizeof(MaterialBufferType);
	buffDesc.Usage = D3D11_USAGE_DEFAULT;

	data.pSysMem = m_materials.data();
	data.SysMemPitch = sizeof(MaterialBufferType);
	data.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&buffDesc, &data, &m_materialBuffer);
	if (FAILED(result))
		return false;

	D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc;
	viewDesc.Buffer.FirstElement = 0;
	viewDesc.Buffer.NumElements = Limits::Materials;
	viewDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

	result = device->CreateUnorderedAccessView(m_materialBuffer, &viewDesc, &m_materialUav);
	if (FAILED(result))
		return false;

	return true;
}

void PMXShader::InternalPrepareRender(ID3D11DeviceContext *context)
{
	context->IASetInputLayout(m_layout);

	UINT count = -1;
	//context->OMSetRenderTargetsAndUnorderedAccessViews
	//context->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, 4, 1, &m_materialUav, &count);

	context->VSSetShader(m_vertexShader, NULL, 0);
	context->PSSetShader(m_pixelShader, NULL, 0);
}

bool PMXShader::InternalRender(ID3D11DeviceContext *context, UINT indexCount, UINT offset)
{
	context->DrawIndexed(indexCount, offset, offset);

	return true;
}

void PMXShader::InternalShutdown()
{
	if (m_materialUav) {
		m_materialUav->Release();
		m_materialUav = nullptr;
	}

	if (m_materialBuffer) {
		m_materialBuffer->Release();
		m_materialBuffer = nullptr;
	}

	if (m_layout) {
		m_layout->Release();
		m_layout = nullptr;
	}

	if (m_vertexShader) {
		m_vertexShader->Release();
		m_vertexShader = nullptr;
	}

	if (m_pixelShader) {
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}
}
