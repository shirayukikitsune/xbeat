#include "LightShader.h"
#include <fstream>

using namespace Renderer::Shaders;

Light::Light(void)
{
	vertexShader = nullptr;
	pixelShader = nullptr;
	layout = nullptr;
	matrixBuffer = nullptr;
	sampleState = nullptr;
	lightBuffer = nullptr;
	cameraBuffer = nullptr;
}


Light::~Light(void)
{
	Shutdown();
}

bool Light::Initialize(ID3D11Device *device, HWND wnd)
{
	if (!InitializeShader(device, wnd, L"./Data/Shaders/LightVertex.cso", L"./Data/Shaders/LightPixel.cso"))
		return false;

	return true;
}

void Light::Shutdown()
{
	ShutdownShader();
}

bool Light::Render(ID3D11DeviceContext *context, int indexCount, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView **textures, int textureCount, DirectX::XMFLOAT3 lightDirection, DirectX::XMFLOAT4 diffuseColor, DirectX::XMFLOAT4 ambientColor, DirectX::XMFLOAT3 cameraPosition, DirectX::XMFLOAT4 specularColor, float specularPower, Renderer::DXType<ID3D11Buffer> materialBuffer, uint32_t materialIndex)
{
	DirectX::XMMATRIX wvp = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(world, view), projection);

	if (!SetMaterialInfo(context, materialIndex))
		return false;

	if (!SetShaderParameters(context, world, wvp, lightDirection, diffuseColor, ambientColor, cameraPosition, specularColor, specularPower, materialBuffer))
		return false;

	RenderShader(context, indexCount, textures, textureCount);
	return true;
}

char *getFileContents(const std::wstring &file, uint64_t &bufSize)
{
	std::ifstream ifs;
	ifs.open(file, std::ios::binary);

	if (ifs.fail())
		return nullptr;

	ifs.seekg(0, std::ios::end);
	bufSize = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	char* buffer = new char[bufSize];
	ifs.read(buffer, bufSize);

	ifs.close();

	return buffer;
}

bool Light::InitializeShader(ID3D11Device *device, HWND wnd, const std::wstring &vsFile, const std::wstring &psFile)
{
	HRESULT result;
#if 0
	D3D11_INPUT_ELEMENT_DESC polygonLayout[] = {
		{ "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// UV[1..4]
		/*{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },*/
	};
	UINT numElements = sizeof (polygonLayout) / sizeof (polygonLayout[0]);
#endif
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC cameraBufferDesc;
	D3D11_BUFFER_DESC lightBufferDesc;
	D3D11_BUFFER_DESC materialInfoBufferDesc;
	uint64_t vsbufsize, psbufsize;

	char *vsbuffer = getFileContents(vsFile, vsbufsize);
	result = device->CreateVertexShader(vsbuffer, vsbufsize, NULL, &vertexShader);
	if (FAILED(result)) {
		delete[] vsbuffer;
		return false;
	}

	char *psbuffer = getFileContents(psFile, psbufsize);
	result = device->CreatePixelShader(psbuffer, psbufsize, NULL, &pixelShader);
	if (FAILED(result)) {
		delete[] vsbuffer;
		delete[] psbuffer;
		return false;
	}

	result = device->CreateInputLayout(DirectX::VertexPositionNormalTexture::InputElements, DirectX::VertexPositionNormalTexture::InputElementCount, vsbuffer, vsbufsize, &layout);
	//result = device->CreateInputLayout(polygonLayout, numElements, vsbuffer, vsbufsize, &layout);
	if (FAILED(result)) {
		return false;
	}

	delete[] vsbuffer;
	vsbuffer = nullptr;

	delete[] psbuffer;
	psbuffer = nullptr;

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

	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof (MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);
	if (FAILED(result))
		return false;

	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof (CameraBufferType);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraBufferDesc.MiscFlags = 0;
	cameraBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&cameraBufferDesc, NULL, &cameraBuffer);
	if (FAILED(result))
		return false;

	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof (LightBufferType);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&lightBufferDesc, NULL, &lightBuffer);
	if (FAILED(result))
		return false;

	materialInfoBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	materialInfoBufferDesc.ByteWidth = sizeof (MaterialInfoType);
	materialInfoBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	materialInfoBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	materialInfoBufferDesc.MiscFlags = 0;
	materialInfoBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&materialInfoBufferDesc, NULL, &materialInfoBuffer);
	if (FAILED(result))
		return false;

	return true;
}

void Light::ShutdownShader() 
{
	if (lightBuffer) {
		lightBuffer->Release();
		lightBuffer = nullptr;
	}

	if (cameraBuffer) {
		cameraBuffer->Release();
		cameraBuffer = nullptr;
	}

	if (sampleState) {
		sampleState->Release();
		sampleState = nullptr;
	}

	if (matrixBuffer) {
		matrixBuffer->Release();
		matrixBuffer = nullptr;
	}

	if (layout) {
		layout->Release();
		layout = nullptr;
	}

	if (pixelShader) {
		pixelShader->Release();
		pixelShader = nullptr;
	}

	if (vertexShader) {
		vertexShader->Release();
		vertexShader = nullptr;
	}
}

void Light::OutputShaderErrorMessage(ID3D10Blob *errorMessage, HWND wnd, const std::wstring &file)
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

	MessageBox(wnd, L"Error compiling shader", file.c_str(), MB_OK);
}

bool Light::SetMaterialInfo(ID3D11DeviceContext *context, uint32_t materialIndex)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MaterialInfoType *data;

	result = context->Map(materialInfoBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return false;

	data = (MaterialInfoType*)mappedResource.pData;

	data->materialIndex = materialIndex;

	context->Unmap(materialInfoBuffer, 0);

	context->PSSetConstantBuffers(2, 1, &materialInfoBuffer);

	return true;
}

bool Light::SetShaderParameters(ID3D11DeviceContext *context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX wvp, DirectX::XMFLOAT3 lightDirection, DirectX::XMFLOAT4 diffuseColor, DirectX::XMFLOAT4 ambientColor, DirectX::XMFLOAT3 cameraPosition, DirectX::XMFLOAT4 specularColor, float specularPower, Renderer::DXType<ID3D11Buffer> materialBuffer)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType *dataPtr;
	LightBufferType *dataPtr2;
	CameraBufferType *dataPtr3;

	result = context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return false;

	dataPtr = (MatrixBufferType*)mappedResource.pData;

	dataPtr->world = DirectX::XMMatrixTranspose(world);
	dataPtr->wvp = DirectX::XMMatrixTranspose(wvp);

	context->Unmap(matrixBuffer, 0);

	context->VSSetConstantBuffers(0, 1, &matrixBuffer);

	result = context->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return false;

	dataPtr3 = (CameraBufferType*)mappedResource.pData;

	(DirectX::XMFLOAT3)dataPtr3->cameraPosition = cameraPosition;

	context->Unmap(cameraBuffer, 0);

	context->VSSetConstantBuffers(1, 1, &cameraBuffer);

	// Mess with the Pixel Shader
	result = context->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return false;

	dataPtr2 = (LightBufferType*)mappedResource.pData;

	dataPtr2->ambientColor = ambientColor;
	dataPtr2->diffuseColor = diffuseColor;
	dataPtr2->lightDirection = lightDirection;
	dataPtr2->specularPower = specularPower;
	dataPtr2->specularColor = specularColor;

	context->Unmap(lightBuffer, 0);

	context->PSSetConstantBuffers(0, 1, &lightBuffer);

	if (materialBuffer != nullptr)
		context->PSSetConstantBuffers(1, 1, &materialBuffer);

	context->IASetInputLayout(layout);

	context->VSSetShader(vertexShader, NULL, 0);
	context->PSSetShader(pixelShader, NULL, 0);

	context->PSSetSamplers(0, 1, &sampleState);

	return true;
}

void Light::RenderShader(ID3D11DeviceContext *context, int indexCount, ID3D11ShaderResourceView **textures, int textureCount)
{
	context->PSSetShaderResources(0, textureCount, textures);
	context->DrawIndexed(indexCount, 0, 0);
}
