#include "ColorShaderClass.h"

#include <fstream>

using namespace Renderer;

extern char *getFileContents(const std::wstring &file, SIZE_T &bufSize);

ColorShaderClass::ColorShaderClass(void)
{
	vertexShader = nullptr;
	pixelShader = nullptr;
	layout = nullptr;
	matrixBuffer = nullptr;
}


ColorShaderClass::~ColorShaderClass(void)
{
}


bool ColorShaderClass::Initialize(ID3D11Device *device, HWND wnd)
{
	if (!InitializeShader(device, wnd, L"./ColorShader.cso", L"./ColorShader.cso"))
		return false;

	return true;
}

void ColorShaderClass::Shutdown()
{
	ShutdownShader();
}

bool ColorShaderClass::Render(ID3D11DeviceContext *context, int indexCount, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	if (!SetShaderParameters(context, world, view, projection))
		return false;

	RenderShader(context, indexCount);
	return true;
}

bool ColorShaderClass::InitializeShader(ID3D11Device *device, HWND wnd, const std::wstring &vsFile, const std::wstring &psFile)
{
	HRESULT result;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	UINT numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;
	SIZE_T vsbufsize, psbufsize;

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

	polygonLayout[1].SemanticName = "COLOR";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
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

	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof (MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);
	if (FAILED(result))
		return false;

	return true;
}

void ColorShaderClass::ShutdownShader() 
{
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

void ColorShaderClass::OutputShaderErrorMessage(ID3D10Blob *errorMessage, HWND wnd, const std::wstring &file)
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

bool ColorShaderClass::SetShaderParameters(ID3D11DeviceContext *context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType *buffer;
	UINT bufferNumber;

	result = context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return false;

	buffer = (MatrixBufferType*)mappedResource.pData;

	buffer->world = DirectX::XMMatrixTranspose(world);
	buffer->view = DirectX::XMMatrixTranspose(view);
	buffer->projection = DirectX::XMMatrixTranspose(projection);

	context->Unmap(matrixBuffer, 0);

	bufferNumber = 0;
	
	context->VSSetConstantBuffers(bufferNumber, 1, &matrixBuffer);

	return true;
}

void ColorShaderClass::RenderShader(ID3D11DeviceContext *context, int indexCount)
{
	context->IASetInputLayout(layout);

	context->VSSetShader(vertexShader, NULL, 0);
	context->PSSetShader(pixelShader, NULL, 0);

	context->DrawIndexed(indexCount, 0, 0);
}
