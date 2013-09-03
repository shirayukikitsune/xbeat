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

namespace Renderer {
class D3DRenderer;

class Model
{
protected:
	struct VertexType {
		DirectX::XMFLOAT3 position; 
		DirectX::XMFLOAT2 texture; 
		DirectX::XMFLOAT3 normal;
	};

	struct ModelType {
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

public:
	Model(void);
	virtual ~Model(void);

	bool Initialize(ID3D11Device* device, const std::wstring &modelfile);
	void Shutdown();
	bool Render(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<CameraClass> camera, std::shared_ptr<ViewFrustum> frustum);

	int GetIndexCount();

	ID3D11ShaderResourceView *GetTexture();

protected:
	virtual bool InitializeBuffers(ID3D11Device* device);
	virtual void ShutdownBuffers();
	virtual bool RenderBuffers(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<CameraClass> camera, std::shared_ptr<ViewFrustum> frustum);

	virtual bool LoadTexture(ID3D11Device *device);
	virtual void ReleaseTexture();

	virtual bool LoadModel(const std::wstring &modelfile);
	virtual void ReleaseModel();

private:
	std::vector<ModelType> geometry;
	std::vector<UINT> indices;
	int vertexCount, indexCount;
	ID3D11Buffer *vertexBuffer, *indexBuffer;

	std::shared_ptr<Texture> texture;
};

}
