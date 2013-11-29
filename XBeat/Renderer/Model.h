#pragma once

#include <D3D11.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "Shaders/LightShader.h"
#include "Texture.h"
#include "Light.h"
#include "CameraClass.h"
#include "ViewFrustum.h"
#include "../Physics/Environment.h"
#include "../Dispatcher.h"

namespace Renderer {
class D3DRenderer;

class Model
{
protected:
#if 0
	struct VertexType {
		VertexType() : position(0,0,0), texture(0,0), normal(0,0,0)/*, UV1(0,0,0,0), UV2(0,0,0,0), UV3(0,0,0,0), UV4(0,0,0,0)*/ {
		}
		DirectX::XMFLOAT3 position; 
		DirectX::XMFLOAT2 texture; 
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT4 UV1; // Extra UV coordinates from PMX
		DirectX::XMFLOAT4 UV2;
		DirectX::XMFLOAT4 UV3;
		DirectX::XMFLOAT4 UV4;
	};
#endif

	struct ModelType {
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

public:
	Model(void);
	virtual ~Model(void);

	bool Initialize(std::shared_ptr<Renderer::D3DRenderer> d3d, const std::wstring &modelfile, std::shared_ptr<Physics::Environment> physics, std::shared_ptr<Dispatcher> dispatcher);
	void Shutdown();
	bool Render(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<CameraClass> camera, std::shared_ptr<ViewFrustum> frustum);

	int GetIndexCount();

	ID3D11ShaderResourceView *GetTexture();

protected:
	virtual bool InitializeBuffers(std::shared_ptr<Renderer::D3DRenderer> d3d);
	virtual void ShutdownBuffers();
	virtual bool RenderBuffers(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<CameraClass> camera, std::shared_ptr<ViewFrustum> frustum);

	virtual bool LoadTexture(ID3D11Device *device);
	virtual void ReleaseTexture();

	virtual bool LoadModel(const std::wstring &modelfile);
	virtual void ReleaseModel();

	std::shared_ptr<Physics::Environment> m_physics;
	std::shared_ptr<Dispatcher> m_dispatcher;
private:
	std::vector<ModelType> geometry;
	std::vector<UINT> indices;
	int vertexCount, indexCount;
	ID3D11Buffer *vertexBuffer, *indexBuffer;

	std::shared_ptr<Texture> texture;
};

}
