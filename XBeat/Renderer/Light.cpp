#include "Light.h"

using namespace Renderer;


Light::Light()
{
}

Light::~Light()
{
}

void Light::SetAmbientColor(float r, float g, float b, float a)
{
	ambientColor = DirectX::XMFLOAT4(r, g, b, a);
}

void Light::SetDiffuseColor(float r, float g, float b, float a)
{
	diffuseColor = DirectX::XMFLOAT4(r, g, b, a);
}

void Light::SetSpecularColor(float r, float g, float b, float a)
{
	specularColor = DirectX::XMFLOAT4(r, g, b, a);
}

void Light::SetDirection(float x, float y, float z)
{
	direction = DirectX::XMFLOAT3(x, y, z);
}

void Light::SetSpecularPower(float val)
{
	specularPower = val;
}

DirectX::XMFLOAT4 Light::GetAmbientColor()
{
	return ambientColor;
}

DirectX::XMFLOAT4 Light::GetDiffuseColor()
{
	return diffuseColor;
}

DirectX::XMFLOAT4 Light::GetSpecularColor()
{
	return specularColor;
}

DirectX::XMFLOAT3 Light::GetDirection()
{
	return direction;
}

float Light::GetSpecularPower()
{
	return specularPower;
}
		