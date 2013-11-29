#include "CameraClass.h"
#include "Shaders/LightShader.h"
#include "Light.h"
#include "Model.h"
#include "D3DRenderer.h"
#include <fstream>

using Renderer::Model;

Model::Model(void)
{
	vertexBuffer = nullptr;
	indexBuffer = nullptr;

	texture = nullptr;

	geometry.resize(0);
}


Model::~Model(void)
{
	Shutdown();
}


bool Model::Initialize(std::shared_ptr<Renderer::D3DRenderer> d3d, const std::wstring &modelfile, std::shared_ptr<Physics::Environment> physics, std::shared_ptr<Dispatcher> dispatcher)
{
	m_physics = physics;
	m_dispatcher = dispatcher;

	if (!LoadModel(modelfile))
		return false;

	if (!InitializeBuffers(d3d))
		return false;

	if (!LoadTexture(d3d->GetDevice()))
		return false;

	return true;
}

void Model::Shutdown()
{
	ReleaseTexture();

	ShutdownBuffers();

	ReleaseModel();
}

bool Model::Render(std::shared_ptr<Renderer::D3DRenderer> d3d, std::shared_ptr<Renderer::Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Renderer::Light> light, std::shared_ptr<Renderer::CameraClass> camera, std::shared_ptr<Renderer::ViewFrustum> frustum)
{
	if (!RenderBuffers(d3d, lightShader, view, projection, world, light, camera, frustum))
		return false;

	return true;
}

int Model::GetIndexCount()
{
	return indexCount;
}

ID3D11ShaderResourceView *Model::GetTexture()
{
	return texture->GetTexture();
}

bool Model::InitializeBuffers(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;

	//VertexType *vertices = new VertexType[vertexCount];
	DirectX::VertexPositionNormalTexture *vertices = new DirectX::VertexPositionNormalTexture[vertexCount];
	if (vertices == nullptr)
		return false;

	for (int i = 0; i < vertexCount; i++) {
		vertices[i].position = DirectX::XMFLOAT3(geometry[i].x, geometry[i].y, geometry[i].z);
		vertices[i].textureCoordinate = DirectX::XMFLOAT2(geometry[i].tu, geometry[i].tv);
		vertices[i].normal = DirectX::XMFLOAT3(geometry[i].nx, geometry[i].ny, geometry[i].nz);
	}

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof (DirectX::VertexPositionNormalTexture) * vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = d3d->GetDevice()->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);
	if (FAILED(result))
		return false;

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof (UINT) * indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = indices.data();
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = d3d->GetDevice()->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
	if(FAILED(result))
		return false;

	delete [] vertices;
	vertices = nullptr;

	return true;
}

bool Model::LoadTexture(ID3D11Device *device)
{
	texture.reset(new Texture);
	if (texture == nullptr)
		return false;

	if (!texture->Initialize(device, L"./Data/Textures/grass.jpg"))
		return false;

	return true;
}

void Model::ShutdownBuffers()
{
	if (indexBuffer) {
		indexBuffer->Release();
		indexBuffer = nullptr;
	}

	if (vertexBuffer) {
		vertexBuffer->Release();
		vertexBuffer = nullptr;
	}
}

void Model::ReleaseTexture()
{
	if (texture != nullptr) {
		texture->Shutdown();
		texture.reset();
	}
}

bool Model::RenderBuffers(std::shared_ptr<Renderer::D3DRenderer> d3d, std::shared_ptr<Renderer::Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Renderer::Light> light, std::shared_ptr<Renderer::CameraClass> camera, std::shared_ptr<Renderer::ViewFrustum> frustum)
{
	unsigned int stride;
	unsigned int offset;
	DirectX::XMFLOAT3 cameraPosition;
	ID3D11DeviceContext *deviceContext = d3d->GetDeviceContext();


	// Set vertex buffer stride and offset.
	stride = sizeof(DirectX::VertexPositionNormalTexture); 
	offset = 0;

	deviceContext->RSSetState(d3d->GetRasterState(1));
    
	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DirectX::XMStoreFloat3(&cameraPosition, camera->GetPosition());

	auto tex = this->GetTexture();
	static DXType<ID3D11Buffer> matBuffer;
	if (!matBuffer) {
		Shaders::Light::MaterialBufferType * matbuf = new Shaders::Light::MaterialBufferType;
		matbuf->flags = 0;
		matbuf->ambientColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		matbuf->diffuseColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		matbuf->morphWeight = 0.0f;
		matbuf->padding = DirectX::XMFLOAT2(0, 0);
		matbuf->specularColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		matbuf->addBaseCoefficient = matbuf->addSphereCoefficient = matbuf->addToonCoefficient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		matbuf->mulBaseCoefficient = matbuf->mulSphereCoefficient = matbuf->mulToonCoefficient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

		D3D11_BUFFER_DESC desc;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = sizeof (Shaders::Light::MaterialBufferType);
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		desc.Usage = D3D11_USAGE_IMMUTABLE;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = matbuf;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		if (FAILED(d3d->GetDevice()->CreateBuffer(&desc, &data, &matBuffer)))
			return false;

		delete matbuf;
	}

	if (!lightShader->Render(deviceContext, this->indexCount, world, view, projection, &tex, 1, light->GetDirection(), light->GetDiffuseColor(), light->GetAmbientColor(), cameraPosition, light->GetSpecularColor(), light->GetSpecularPower(), matBuffer, -1))
		return false;

	deviceContext->RSSetState(d3d->GetRasterState(0));
    
	return true;
}

bool Model::LoadModel(const std::wstring &filename)
{
	std::ifstream fin;
	fin.open(filename);
	if (fin.fail())
		return false;

	fin >> vertexCount;
	indexCount = vertexCount;

	geometry.resize(vertexCount);
	indices.resize(indexCount);

	for (int i = 0; i < vertexCount; i++) {
		fin >> geometry[i].x >> geometry[i].y >> geometry[i].z;
		fin >> geometry[i].tu >> geometry[i].tv;
		fin >> geometry[i].nx >> geometry[i].ny >> geometry[i].nz;

		indices[i] = i;
	}

	fin.close();

	return true;
}

void Model::ReleaseModel()
{
	geometry.clear();
	geometry.resize(0);
}
