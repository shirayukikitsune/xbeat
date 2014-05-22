#include "PMXShader.h"
#include <DirectXMath.h>

using namespace DirectX;

using Renderer::PMX::PMXShader;

// This buffer is set per material
/*struct ShaderCBuffer {
	XMVECTOR diffuseColor;
	XMVECTOR ambientColor;
	XMVECTOR specularColor;

	XMVECTOR lightDirection;
	XMVECTOR lightDiffuseColor;
	XMVECTOR lightSpecularColor;

	XMVECTOR eyePosition;

	XMVECTOR fogColor;
	XMVECTOR fogVector;

	XMMATRIX world;
	XMVECTOR worldInverseTranspose[3];
	XMMATRIX worldViewProj;

	XMVECTOR bones[MaxBones][3];
};*/


PMXShader::PMXShader()
{
}


PMXShader::~PMXShader()
{
}
