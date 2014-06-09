#include "Light.h"

using namespace Renderer;


Light::Light()
	: ambientColor(1.0f, 1.0f, 1.0f, 1.0f),
	diffuseColor(1.0f, 1.0f, 1.0f, 1.0f),
	specularColor(1.0f, 1.0f, 1.0f),
	specularPower(0.0f),
	direction(0.0f, 0.0f, 0.0f)
{
}

void XM_CALLCONV Light::SetAmbientColor(float r, float g, float b, float a)
{
	ambientColor = DirectX::XMFLOAT4(r, g, b, a);
}

void XM_CALLCONV Light::SetDiffuseColor(float r, float g, float b, float a)
{
	diffuseColor = DirectX::XMFLOAT4(r, g, b, a);
}

void XM_CALLCONV Light::SetSpecularColor(float r, float g, float b)
{
	specularColor = DirectX::XMFLOAT3(r, g, b);
}

void XM_CALLCONV Light::SetDirection(float x, float y, float z)
{
	direction = DirectX::XMFLOAT3(x, y, z);
}

void XM_CALLCONV Light::SetSpecularPower(float val)
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

DirectX::XMFLOAT3 Light::GetSpecularColor()
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
		