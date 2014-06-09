#include "Camera.h"
#include "Shaders/LightShader.h"
#include "Light.h"
#include "Model.h"
#include "DXUtil.h"
#include "D3DRenderer.h"
#include "ViewFrustum.h"

using Renderer::Model;

Model::Model(void)
{
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

void Model::Render(ID3D11DeviceContext *context, std::shared_ptr<Renderer::ViewFrustum> frustum)
{
}

bool Model::InitializeBuffers(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	return true;
}

bool Model::LoadTexture(ID3D11Device *device)
{
	return true;
}

void Model::ShutdownBuffers()
{
}

void Model::ReleaseTexture()
{
}

bool Model::RenderBuffers(std::shared_ptr<Renderer::D3DRenderer> d3d, std::shared_ptr<Renderer::Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Renderer::Light> light, std::shared_ptr<Renderer::Camera> camera, std::shared_ptr<Renderer::ViewFrustum> frustum)
{
	return true;
}

bool Model::LoadModel(const std::wstring &filename)
{
	return true;
}

void Model::ReleaseModel()
{
}
