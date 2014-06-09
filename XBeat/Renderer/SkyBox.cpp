#include "SkyBox.h"
#include "Texture.h"
#include "Shaders/GenericShader.h"

using namespace Renderer;

SkyBox::SkyBox(void)
{
}


SkyBox::~SkyBox(void)
{
	Shutdown();
}

bool SkyBox::Initialize(std::shared_ptr<D3DRenderer> d3d, const std::wstring &texturefile, std::shared_ptr<Light> light, HWND wnd)
{
	if (!InitializeBuffers(d3d->GetDevice(), light, wnd, L"./Data/Shaders/SkyboxVertex.cso", L"./Data/Shaders/SkyboxPixel.cso"))
		return false;

	if (!LoadModel(d3d, 10, 10))
		return false;

	if (!LoadTexture(d3d->GetDevice(), texturefile))
		return false;

	return true;
}

void SkyBox::Shutdown()
{
	ReleaseModel();
	ReleaseTexture();
	ShutdownBuffers();
}

bool SkyBox::Render(std::shared_ptr<D3DRenderer> d3d, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, std::shared_ptr<Camera> camera, std::shared_ptr<Light> light)
{
	ID3D11DeviceContext *context = d3d->GetDeviceContext();
	DirectX::XMMATRIX sphereWorld = DirectX::XMMatrixIdentity(), scale = DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f), translation;

	translation = DirectX::XMMatrixTranslationFromVector(camera->GetPosition());
	sphereWorld = scale * translation;

	if (!SetShaderParameters(context, sphereWorld, view, projection, texture->GetTexture()))
		return false;

	UINT strides = sizeof(DirectX::VertexPositionNormalTexture), offset = 0;

	context->IASetInputLayout(m_layout);
	context->IASetIndexBuffer(sphereIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetVertexBuffers(0, 1, &sphereVertBuffer, &strides, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	context->PSSetShader(pixelShader, NULL, 0);
	context->VSSetShader(vertexShader, NULL, 0);
	context->RSSetState(d3d->GetRasterState(1));

	d3d->SetDepthLessEqual();

	context->DrawIndexed(NumSphereFaces*3, 0, 0);

	d3d->End2D();

	context->RSSetState(d3d->GetRasterState(0));

	return true;
}

bool SkyBox::SetShaderParameters(ID3D11DeviceContext *context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView *texture)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType *buffer;
	UINT bufferNumber;

	result = context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return false;

	buffer = (MatrixBufferType*)mappedResource.pData;

	buffer->wvp = world * view * projection;
	buffer->wvp = DirectX::XMMatrixTranspose(buffer->wvp);

	context->Unmap(matrixBuffer, 0);

	bufferNumber = 0;
	
	context->VSSetConstantBuffers(bufferNumber, 1, &matrixBuffer);

	context->PSSetShaderResources(0, 1, &texture);
	context->PSSetSamplers(0, 1, &sampleState);

	return true;
}

bool SkyBox::InitializeBuffers(ID3D11Device* device, std::shared_ptr<Light> light, HWND wnd, const std::wstring &vsFile, const std::wstring &psFile)
{
	HRESULT result;
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	SIZE_T vsbufsize, psbufsize;

	char *vsbuffer = Shaders::Generic::getFileContents(vsFile, vsbufsize);
	result = device->CreateVertexShader(vsbuffer, vsbufsize, NULL, &vertexShader);
	if (FAILED(result)) {
		delete[] vsbuffer;
		return false;
	}

	char *psbuffer = Shaders::Generic::getFileContents(psFile, psbufsize);
	result = device->CreatePixelShader(psbuffer, psbufsize, NULL, &pixelShader);
	if (FAILED(result)) {
		delete[] vsbuffer;
		delete[] psbuffer;
		return false;
	}

	result = device->CreateInputLayout(DirectX::VertexPositionNormalTexture::InputElements, DirectX::VertexPositionNormalTexture::InputElementCount, vsbuffer, vsbufsize, &m_layout);
	delete[] psbuffer;
	delete[] vsbuffer;
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

	ZeroMemory(&samplerDesc, sizeof (samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	result = device->CreateSamplerState(&samplerDesc, &sampleState);
	if (FAILED(result))
		return false;

	return true;
}

void SkyBox::ShutdownBuffers()
{
	DX_DELETEIF(sampleState);
	DX_DELETEIF(matrixBuffer);
	DX_DELETEIF(m_layout);
	DX_DELETEIF(vertexShader);
	DX_DELETEIF(pixelShader);
}

bool SkyBox::LoadTexture(ID3D11Device *device, const std::wstring &file)
{
	texture.reset(new Texture);
	if (texture == nullptr)
		return false;

	if (!texture->Initialize(device, file))
		return false;

	return true;
}

void SkyBox::ReleaseTexture()
{
	texture.reset();
}

bool SkyBox::LoadModel(std::shared_ptr<D3DRenderer> d3d, int latLines, int longLines)
{
	NumSphereVertices = ((latLines-2) * longLines) + 2;
	NumSphereFaces  = ((latLines-3)*(longLines)*2) + (longLines*2);

	float sphereYaw = 0.0f;
	float spherePitch = 0.0f;
	DirectX::XMMATRIX Rotationx, Rotationy;

	std::vector<DirectX::VertexPositionNormalTexture> vertices(NumSphereVertices);

	DirectX::XMVECTOR currVertPos = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	vertices[0].position.x = 0.0f;
	vertices[0].position.y = 0.0f;
	vertices[0].position.z = 1.0f;

	for(int i = 0; i < latLines-2; ++i)
	{
		spherePitch = (i+1) * (DirectX::XM_PI/(latLines-1));
		Rotationx = DirectX::XMMatrixRotationX(spherePitch);
		for(int j = 0; j < longLines; ++j)
		{
			sphereYaw = j * (DirectX::XM_2PI/(longLines));
			Rotationy = DirectX::XMMatrixRotationZ(sphereYaw);
			currVertPos = DirectX::XMVector3TransformNormal( DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (Rotationx * Rotationy) );	
			currVertPos = DirectX::XMVector3Normalize( currVertPos );
			vertices[i*longLines+j+1].position.x = DirectX::XMVectorGetX(currVertPos);
			vertices[i*longLines+j+1].position.y = DirectX::XMVectorGetY(currVertPos);
			vertices[i*longLines+j+1].position.z = DirectX::XMVectorGetZ(currVertPos);
		}
	}

	vertices[NumSphereVertices-1].position.x =  0.0f;
	vertices[NumSphereVertices-1].position.y =  0.0f;
	vertices[NumSphereVertices-1].position.z = -1.0f;

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory( &vertexBufferDesc, sizeof(vertexBufferDesc) );

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof( DirectX::VertexPositionNormalTexture ) * NumSphereVertices;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData; 

	ZeroMemory( &vertexBufferData, sizeof(vertexBufferData) );
	vertexBufferData.pSysMem = &vertices[0];
	HRESULT hr = d3d->GetDevice()->CreateBuffer( &vertexBufferDesc, &vertexBufferData, &sphereVertBuffer);
	if (FAILED(hr))
		return false;

	std::vector<DWORD> indices(NumSphereFaces * 3);

	int k = 0;
	for(int l = 0; l < longLines-1; ++l)
	{
		indices[k] = 0;
		indices[k+1] = l+1;
		indices[k+2] = l+2;
		k += 3;
	}

	indices[k] = 0;
	indices[k+1] = longLines;
	indices[k+2] = 1;
	k += 3;

	for(int i = 0; i < latLines-3; ++i)
	{
		for(int j = 0; j < longLines-1; ++j)
		{
			indices[k]   = i*longLines+j+1;
			indices[k+1] = i*longLines+j+2;
			indices[k+2] = (i+1)*longLines+j+1;

			indices[k+3] = (i+1)*longLines+j+1;
			indices[k+4] = i*longLines+j+2;
			indices[k+5] = (i+1)*longLines+j+2;

			k += 6; // next quad
		}

		indices[k]   = (i*longLines)+longLines;
		indices[k+1] = (i*longLines)+1;
		indices[k+2] = ((i+1)*longLines)+longLines;

		indices[k+3] = ((i+1)*longLines)+longLines;
		indices[k+4] = (i*longLines)+1;
		indices[k+5] = ((i+1)*longLines)+1;

		k += 6;
	}

	for(int l = 0; l < longLines-1; ++l)
	{
		indices[k] = NumSphereVertices-1;
		indices[k+1] = (NumSphereVertices-1)-(l+1);
		indices[k+2] = (NumSphereVertices-1)-(l+2);
		k += 3;
	}

	indices[k] = NumSphereVertices-1;
	indices[k+1] = (NumSphereVertices-1)-longLines;
	indices[k+2] = NumSphereVertices-2;

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory( &indexBufferDesc, sizeof(indexBufferDesc) );

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(DWORD) * NumSphereFaces * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;

	iinitData.pSysMem = &indices[0];
	hr = d3d->GetDevice()->CreateBuffer(&indexBufferDesc, &iinitData, &sphereIndexBuffer);
	if (FAILED(hr))
		return false;

	return true;
}

void SkyBox::ReleaseModel()
{
	DX_DELETEIF(sphereIndexBuffer);
	DX_DELETEIF(sphereVertBuffer);
}
