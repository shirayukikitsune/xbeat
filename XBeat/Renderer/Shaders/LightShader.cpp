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

bool Light::Render(ID3D11DeviceContext *context, int indexCount, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView **textures, int textureCount, DirectX::XMFLOAT3 lightDirection, DirectX::XMFLOAT4 diffuseColor, DirectX::XMFLOAT4 ambientColor, DirectX::XMFLOAT3 cameraPosition, DirectX::XMFLOAT4 specularColor, float specularPower, MaterialBufferType &materialInfo)
{
	if (!SetShaderParameters(context, world, view, projection, textures, textureCount, lightDirection, diffuseColor, ambientColor, cameraPosition, specularColor, specularPower, materialInfo))
		return false;

	RenderShader(context, indexCount);
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
	D3D11_INPUT_ELEMENT_DESC polygonLayout[3];
	UINT numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC cameraBufferDesc;
	D3D11_BUFFER_DESC lightBufferDesc;
	D3D11_BUFFER_DESC materialBufferDesc;
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

	polygonLayout[2].SemanticName = "NORMAL";
	polygonLayout[2].SemanticIndex = 0;
	polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[2].InputSlot = 0;
	polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate = 0;

	numElements = sizeof (polygonLayout) / sizeof (polygonLayout[0]);

	result = device->CreateInputLayout(polygonLayout, numElements, vsbuffer, vsbufsize, &layout);
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

	materialBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	materialBufferDesc.ByteWidth = sizeof (MaterialBufferType);
	materialBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	materialBufferDesc.CPUAccessFlags =  D3D11_CPU_ACCESS_WRITE;
	materialBufferDesc.MiscFlags = 0;
	materialBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&materialBufferDesc, NULL, &materialBuffer);
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
	SIZE_T bufferSize, i;
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

bool Light::SetShaderParameters(ID3D11DeviceContext *context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView **textures, int textureCount, DirectX::XMFLOAT3 lightDirection, DirectX::XMFLOAT4 diffuseColor, DirectX::XMFLOAT4 ambientColor, DirectX::XMFLOAT3 cameraPosition, DirectX::XMFLOAT4 specularColor, float specularPower, MaterialBufferType &materialInfo)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType *dataPtr;
	LightBufferType *dataPtr2;
	CameraBufferType *dataPtr3;
	MaterialBufferType *dataPtr4;

	result = context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return false;

	dataPtr = (MatrixBufferType*)mappedResource.pData;

	dataPtr->world = DirectX::XMMatrixTranspose(world);
	dataPtr->view = DirectX::XMMatrixTranspose(view);
	dataPtr->projection = DirectX::XMMatrixTranspose(projection);

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
	context->PSSetShaderResources(0, textureCount, textures);

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

	result = context->Map(materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return false;

	dataPtr4 = (MaterialBufferType*)mappedResource.pData;

	dataPtr4->ambientColor = materialInfo.ambientColor;
	dataPtr4->diffuseColor = materialInfo.diffuseColor;
	dataPtr4->flags = materialInfo.flags;
	dataPtr4->padding = materialInfo.padding;
	dataPtr4->specularColor = materialInfo.specularColor;

	context->Unmap(materialBuffer, 0);

	context->PSSetConstantBuffers(1, 1, &materialBuffer);

	return true;
}

void Light::RenderShader(ID3D11DeviceContext *context, int indexCount)
{
	context->IASetInputLayout(layout);

	context->VSSetShader(vertexShader, NULL, 0);
	context->PSSetShader(pixelShader, NULL, 0);

	context->PSSetSamplers(0, 1, &sampleState);

	context->DrawIndexed(indexCount, 0, 0);
}
