#pragma once
#include "../Model.h"
#include "../../Physics/Environment.h"

namespace Renderer {
class OBJModel :
	public Model
{
public:
	struct Face {
		Face(){};
		~Face(){};

		std::vector<int32_t> vertices;
		std::vector<int32_t> uvs;
		std::vector<int32_t> normals;
	};

	OBJModel();
	~OBJModel();

	virtual void Render(ID3D11DeviceContext *context, std::shared_ptr<ViewFrustum> frustum);

private:
	std::vector<DirectX::XMVECTOR> m_vertices;
	std::vector<DirectX::XMVECTOR> m_uvs;
	std::vector<DirectX::XMVECTOR> m_normals;
	std::vector<std::shared_ptr<Face>> m_faces;
	
	std::shared_ptr<Texture> m_texture;

	ID3D11Buffer *m_vertexBuffer, *m_indexBuffer;
	std::shared_ptr<btCollisionShape> m_shape;
	std::shared_ptr<btTriangleIndexVertexArray> m_mesh;
	std::shared_ptr<btRigidBody> m_body;
	int m_indexCount;
	int m_vertexCount;

protected:
	virtual bool InitializeBuffers(std::shared_ptr<Renderer::D3DRenderer> d3d);
	virtual void ShutdownBuffers();
	//virtual bool RenderBuffers(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<Camera> camera, std::shared_ptr<ViewFrustum> frustum);

	virtual bool LoadTexture(ID3D11Device *device);
	virtual void ReleaseTexture();

	virtual bool LoadModel(const std::wstring &modelfile);
	virtual void ReleaseModel();
};
}
