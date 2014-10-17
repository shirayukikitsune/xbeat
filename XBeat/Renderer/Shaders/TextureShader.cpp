#include "Texture.h"
#include "GenericShader.h"

using namespace Renderer::Shaders;

extern char *getFileContents(const std::wstring &file, SIZE_T &bufSize);

Texture::Texture(void)
{
	vertexShader = nullptr;
	pixelShader = nullptr;
	layout = nullptr;
	sampleState = nullptr;
}


Texture::~Texture(void)
{
	Shutdown();
}

bool Texture::Initialize(ID3D11Device *device, HWND wnd)
{
	if (!InitializeShader(device, wnd, L"./Data/Shaders/TextureVertex.cso", L"./Data/Shaders/TexturePixel.cso"))
		return false;

	return true;
}

void Texture::Shutdown()
{
	ShutdownShader();
}

bool Texture::Render(ID3D11DeviceContext *context, int indexCount, ID3D11ShaderResourceView *texture)
{
	if (!SetShaderParameters(context, texture))
		return false;

	RenderShader(context, indexCount);
	return true;
}

bool Texture::InitializeShader(ID3D11Device *device, HWND wnd, const std::wstring &vsFile, const std::wstring &psFile)
{
	HRESULT result;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	UINT numElements;
	D3D11_SAMPLER_DESC samplerDesc;
	SIZE_T vsbufsize, psbufsize;

	char *vsbuffer = Generic::getFileContents(vsFile, vsbufsize);
	result = device->CreateVertexShader(vsbuffer, vsbufsize, NULL, &vertexShader);
	if (FAILED(result)) {
		delete[] vsbuffer;
		return false;
	}

	char *psbuffer = Generic::getFileContents(psFile, psbufsize);
	result = device->CreatePixelShader(psbuffer, psbufsize, NULL, &pixelShader);
	if (FAILED(result)) {
		delete[] vsbuffer;
		delete[] psbuffer;
		return false;
	}

	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "TEXCOORD";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	numElements = sizeof (polygonLayout) / sizeof (polygonLayout[0]);

	result = device->CreateInputLayout(polygonLayout, numElements, vsbuffer, vsbufsize, &layout);
	delete[] vsbuffer;
	delete[] psbuffer; 
	if (FAILED(result)) {
		return false;
	}

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

	result = device->CreateSamplerState(&samplerDesc, &sampleState);
	if (FAILED(result))
		return false;

	return true;
}

void Texture::ShutdownShader() 
{
	DX_DELETEIF(sampleState);
	DX_DELETEIF(layout);
	DX_DELETEIF(pixelShader);
	DX_DELETEIF(vertexShader);
}

bool Texture::SetShaderParameters(ID3D11DeviceContext *context, ID3D11ShaderResourceView *texture)
{
	context->PSSetShaderResources(0, 1, &texture);

	return true;
}

void Texture::RenderShader(ID3D11DeviceContext *context, int indexCount)
{
	context->IASetInputLayout(layout);

	context->VSSetShader(vertexShader, NULL, 0);
	context->PSSetShader(pixelShader, NULL, 0);

	context->PSSetSamplers(0, 1, &sampleState);

	context->DrawIndexed(indexCount, 0, 0);
}
