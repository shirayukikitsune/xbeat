#include "PMXShader.h"
#include <d3dcompiler.h>
#include "../Entity.h"

using namespace DirectX;
using namespace Renderer;
using Renderer::PMX::PMXShader;

bool PMXShader::UpdateMaterialBuffer(ID3D11DeviceContext *context)
{
	D3D11_MAPPED_SUBRESOURCE data;
	HRESULT result;
	
	result = context->Map(m_tmpMatBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &data);
	if (FAILED(result))
		return false;

	memcpy(data.pData, m_materials.data(), sizeof(MaterialBufferType) * m_materials.size());

	context->Unmap(m_tmpMatBuffer, 0);

	context->CopyResource(m_materialBuffer, m_tmpMatBuffer);

	return true;
}

bool PMXShader::UpdateBoneBuffer(ID3D11DeviceContext *context)
{
	D3D11_MAPPED_SUBRESOURCE data;
	HRESULT result;

	result = context->Map(m_tmpBoneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &data);
	if (FAILED(result))
		return false;

	memcpy(data.pData, m_bones.data(), sizeof(BoneBufferType) * m_bones.size());

	context->Unmap(m_tmpBoneBuffer, 0);

	context->CopyResource(m_bonesBuffer, m_tmpBoneBuffer);

	return true;
}

bool PMXShader::InternalInitializeBuffers(ID3D11Device *device, HWND hwnd)
{
	D3D11_BUFFER_DESC buffDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
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

	buffDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	buffDesc.ByteWidth = sizeof(MaterialBufferType) * Limits::Materials;
	buffDesc.CPUAccessFlags = 0;
	buffDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	buffDesc.StructureByteStride = sizeof(MaterialBufferType);
	buffDesc.Usage = D3D11_USAGE_DEFAULT;

	data.pSysMem = m_materials.data();
	data.SysMemPitch = sizeof(MaterialBufferType);
	data.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&buffDesc, &data, &m_materialBuffer);
	if (FAILED(result))
		return false;

	buffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffDesc.ByteWidth = sizeof(MaterialBufferType) * Limits::Materials;
	buffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffDesc.MiscFlags = 0;
	buffDesc.StructureByteStride = 0;
	buffDesc.Usage = D3D11_USAGE_DYNAMIC;

	result = device->CreateBuffer(&buffDesc, nullptr, &m_tmpMatBuffer);
	if (FAILED(result))
		return false;

	viewDesc.BufferEx.FirstElement = 0;
	viewDesc.BufferEx.NumElements = Limits::Materials;
	viewDesc.BufferEx.Flags = 0;
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;

	result = device->CreateShaderResourceView(m_materialBuffer, &viewDesc, &m_materialSrv);
	if (FAILED(result))
		return false;

	buffDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	buffDesc.ByteWidth = sizeof(BoneBufferType) * Limits::Bones;
	buffDesc.CPUAccessFlags = 0;
	buffDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	buffDesc.StructureByteStride = sizeof(BoneBufferType);
	buffDesc.Usage = D3D11_USAGE_DEFAULT;

	data.pSysMem = m_bones.data();
	data.SysMemPitch = sizeof(BoneBufferType);
	data.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&buffDesc, &data, &m_bonesBuffer);
	if (FAILED(result))
		return false;

	buffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffDesc.ByteWidth = sizeof(BoneBufferType) * Limits::Bones;
	buffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffDesc.MiscFlags = 0;
	buffDesc.StructureByteStride = 0;
	buffDesc.Usage = D3D11_USAGE_DYNAMIC;

	result = device->CreateBuffer(&buffDesc, nullptr, &m_tmpBoneBuffer);
	if (FAILED(result))
		return false;

	viewDesc.BufferEx.FirstElement = 0;
	viewDesc.BufferEx.NumElements = Limits::Bones;
	viewDesc.BufferEx.Flags = 0;
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;

	result = device->CreateShaderResourceView(m_bonesBuffer, &viewDesc, &m_bonesSrv);
	if (FAILED(result))
		return false;

#ifdef DEBUG
	m_layout->SetPrivateData(WKPDID_D3DDebugObjectName, 9, "PMXShader");

	m_materialBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, 14, "PMX Mat Buffer");
	m_tmpMatBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, 14, "PMX TmpMat Buffer");
	m_materialSrv->SetPrivateData(WKPDID_D3DDebugObjectName, 11, "PMX Mat SRV");

	m_bonesBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, 14, "PMX Bones Buffer");
	m_tmpBoneBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, 14, "PMX TmpBones Buffer");
	m_bonesSrv->SetPrivateData(WKPDID_D3DDebugObjectName, 11, "PMX Bones SRV");

	m_pixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, 6, "PMX PS");
	m_vertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, 6, "PMX VS");
#endif

	return true;
}

void PMXShader::InternalPrepareRender(ID3D11DeviceContext *context)
{
	context->IASetInputLayout(m_layout);

	context->VSSetShader(m_vertexShader, NULL, 0);
	context->PSSetShader(m_pixelShader, NULL, 0);

	context->PSSetShaderResources(4, 1, &m_materialSrv);
	context->VSSetShaderResources(0, 1, &m_bonesSrv);
}

bool PMXShader::InternalRender(ID3D11DeviceContext *context, UINT indexCount, UINT offset)
{
	context->DrawIndexed(indexCount, offset, offset);

	return true;
}

void PMXShader::InternalShutdown()
{
	if (m_materialSrv) {
		m_materialSrv->Release();
		m_materialSrv = nullptr;
	}

	if (m_tmpMatBuffer) {
		m_tmpMatBuffer->Release();
		m_tmpMatBuffer = nullptr;
	}

	if (m_materialBuffer) {
		m_materialBuffer->Release();
		m_materialBuffer = nullptr;
	}

	if (m_bonesSrv) {
		m_bonesSrv->Release();
		m_bonesSrv = nullptr;
	}

	if (m_tmpBoneBuffer) {
		m_tmpBoneBuffer->Release();
		m_tmpBoneBuffer = nullptr;
	}

	if (m_bonesBuffer) {
		m_bonesBuffer->Release();
		m_bonesBuffer = nullptr;
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
