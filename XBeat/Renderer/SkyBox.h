#pragma once
#include "DXUtil.h"
#include "D3DRenderer.h"
#include "Shaders/Texture.h"
#include "Camera.h"
#include "Light.h"
#include "ViewFrustum.h"
#include "VertexTypes.h"
#include "Effects.h"
#include "Texture.h"
#include "GeometricPrimitive.h"

#include <string>
#include <memory>
#include <vector>

namespace Renderer {

class SkyBox
{
public:
	SkyBox(void);
	~SkyBox(void);

	bool Initialize(std::shared_ptr<D3DRenderer> d3d, const std::wstring &texturefile, std::shared_ptr<Light> light, HWND wnd);
	void Shutdown();
	bool Render(std::shared_ptr<D3DRenderer> d3d, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, std::shared_ptr<Camera> camera, std::shared_ptr<Light> light);

private:
	struct MatrixBufferType {
		DirectX::XMMATRIX wvp;
	};

	bool InitializeBuffers(ID3D11Device* device, std::shared_ptr<Light> light, HWND wnd, const std::wstring &vsFile, const std::wstring &psFile);
	bool SetShaderParameters(ID3D11DeviceContext *context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView *texture);
	void ShutdownBuffers();

	bool LoadTexture(ID3D11Device *device, const std::wstring &file);
	void ReleaseTexture();

	bool LoadModel(std::shared_ptr<D3DRenderer> d3d, int latLines, int longLines);
	void ReleaseModel();

	std::unique_ptr<DirectX::GeometricPrimitive> m_sphere;
	ID3D11InputLayout *m_layout;
	std::unique_ptr<Texture> texture;

	ID3D11VertexShader *vertexShader;
	ID3D11PixelShader *pixelShader;
	ID3D11Buffer *matrixBuffer;
	ID3D11SamplerState *sampleState;
	ID3D11Buffer *sphereIndexBuffer;
	ID3D11Buffer *sphereVertBuffer;

	int NumSphereVertices, NumSphereFaces;
};

}