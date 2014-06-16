#include "GenericShader.h"

#include <fstream>

using namespace Renderer::Shaders;

void XM_CALLCONV Generic::SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	m_cbuffer.matrix.world = DirectX::XMMatrixTranspose(world);
	m_cbuffer.matrix.view = DirectX::XMMatrixTranspose(view);
	m_cbuffer.matrix.projection = DirectX::XMMatrixTranspose(projection);
	m_cbuffer.matrix.wvp = DirectX::XMMatrixTranspose(DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(world, view), projection));
	m_cbuffer.matrix.worldInverse = DirectX::XMMatrixInverse(nullptr, world);
}

void XM_CALLCONV Generic::SetEyePosition(DirectX::FXMVECTOR eyePosition)
{
	m_cbuffer.eyePosition = eyePosition;
}

void XM_CALLCONV Generic::SetLights(DirectX::FXMVECTOR ambient, DirectX::FXMVECTOR diffuse, DirectX::FXMVECTOR specular, DirectX::GXMVECTOR direction, DirectX::HXMVECTOR position, int index)
{
	m_cbuffer.lights[index].ambient = ambient;
	m_cbuffer.lights[index].diffuse = diffuse;
	m_cbuffer.lights[index].specularColorAndPower = specular;
	m_cbuffer.lights[index].direction = direction;
	m_cbuffer.lights[index].position = position;
}

void XM_CALLCONV Generic::SetLightCount(UINT count)
{
	m_cbuffer.lightCount = count;
}

bool Generic::InitializeBuffers(ID3D11Device *device, HWND hwnd)
{
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA data;
	D3D11_SAMPLER_DESC samplerDesc;
	HRESULT hr;

	if (!device)
		return false;

	// Using dynamic because camera and matrices should be updated every frame
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.ByteWidth = sizeof(ConstBuffer);
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.MiscFlags = 0;
	cbufferDesc.StructureByteStride = 0;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	data.pSysMem = &m_cbuffer;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	hr = device->CreateBuffer(&cbufferDesc, &data, &m_dxcbuffer);
	if (FAILED(hr))
		return false;

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = device->CreateSamplerState(&samplerDesc, &m_sampleState);
	if (FAILED(hr))
		return false;

#ifdef DEBUG
	m_dxcbuffer->SetPrivateData(WKPDID_D3DDebugObjectName, 16, "GenericShader CB");
	m_sampleState->SetPrivateData(WKPDID_D3DDebugObjectName, 16, "GenericShader SS");
#endif

	// Allow derived classes to initialize their own buffers
	return InternalInitializeBuffers(device, hwnd);
}

bool Generic::Update(float time, ID3D11DeviceContext *context)
{
	D3D11_MAPPED_SUBRESOURCE map;
	HRESULT hr = context->Map(m_dxcbuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &map);

	if (FAILED(hr))
		return false;

	memcpy(map.pData, &m_cbuffer, sizeof(ConstBuffer));

	context->Unmap(m_dxcbuffer, 0);

	return InternalUpdate(time, context);
}

void Generic::PrepareRender(ID3D11DeviceContext *context)
{
	InternalPrepareRender(context);

	context->VSSetConstantBuffers(0, 1, &m_dxcbuffer);

	context->PSSetConstantBuffers(0, 1, &m_dxcbuffer);
	context->PSSetSamplers(0, 1, &m_sampleState);
}

bool Generic::Render(ID3D11DeviceContext *context, UINT indexCount, UINT offset)
{
	return InternalRender(context, indexCount, offset);
}

void Generic::Shutdown()
{
	if (m_dxcbuffer) {
		m_dxcbuffer->Release();
		m_dxcbuffer = nullptr;
	}

	if (m_sampleState) {
		m_sampleState->Release();
		m_sampleState = nullptr;
	}

	InternalShutdown();
}

void Generic::OutputShaderErrorMessage(ID3DBlob *errorMessage, HWND wnd, const std::wstring &file)
{
	char *compileError;
	SIZE_T bufferSize;
	std::ofstream fout;

	compileError = (char*)errorMessage->GetBufferPointer();
	bufferSize = errorMessage->GetBufferSize();

	fout.open("shader-errors.log", std::ios::app);
	if (fout.bad()) {
		errorMessage->Release();
		return;
	}

	fout.write(compileError, bufferSize);

	fout.close();

	errorMessage->Release();

	MessageBox(wnd, L"Error compiling shader\nSee \"shader-errors.log\" for more information", file.c_str(), MB_OK);
}

char* Generic::getFileContents(const std::wstring &file, SIZE_T &bufSize)
{
	std::ifstream ifs;
	ifs.open(file, std::ios::binary);

	if (ifs.fail())
		return nullptr;

	ifs.seekg(0, std::ios::end);
	bufSize = (SIZE_T)ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	char* buffer = new char[bufSize];
	ifs.read(buffer, bufSize);

	ifs.close();

	return buffer;
}
