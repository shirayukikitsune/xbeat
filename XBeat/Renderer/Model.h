#pragma once

#include <D3D11.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "Shaders/LightShader.h"
#include "Texture.h"
#include "Light.h"
#include "Camera.h"
#include "ViewFrustum.h"
#include "../Physics/Environment.h"
#include "../Dispatcher.h"

namespace Renderer {
class D3DRenderer;

class Model
	: public Entity
{
public:
	Model(void);
	virtual ~Model(void);

	virtual bool LoadModel(const std::wstring &modelfile);
	bool Initialize(std::shared_ptr<Renderer::D3DRenderer> d3d, std::shared_ptr<Physics::Environment> physics);
	void Shutdown();
	virtual void Render(ID3D11DeviceContext *context, std::shared_ptr<ViewFrustum> frustum);

protected:
	virtual bool InitializeBuffers(std::shared_ptr<Renderer::D3DRenderer> d3d);
	virtual void ShutdownBuffers();
	virtual bool RenderBuffers(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<Camera> camera, std::shared_ptr<ViewFrustum> frustum);

	virtual bool LoadTexture(ID3D11Device *device);
	virtual void ReleaseTexture();

	virtual void ReleaseModel();
};

}
