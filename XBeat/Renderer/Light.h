#pragma once

#include "Entity.h"

namespace Renderer {

	class Light
		: public Entity
	{
	public:
		Light();

		void XM_CALLCONV SetAmbientColor(float r, float g, float b, float a);
		void XM_CALLCONV SetDiffuseColor(float r, float g, float b, float a);
		void XM_CALLCONV SetSpecularColor(float r, float g, float b);
		void XM_CALLCONV SetDirection(float x, float y, float z);
		void XM_CALLCONV SetSpecularPower(float p);

		DirectX::XMVECTOR XM_CALLCONV GetAmbientColor();
		DirectX::XMVECTOR XM_CALLCONV GetDiffuseColor();
		DirectX::XMVECTOR XM_CALLCONV GetSpecularColor();
		DirectX::XMVECTOR XM_CALLCONV GetDirection();
		float XM_CALLCONV GetSpecularPower();

	private:
		DirectX::XMFLOAT4 ambientColor;
		DirectX::XMFLOAT4 diffuseColor;
		DirectX::XMFLOAT3 specularColor;
		float specularPower;
		DirectX::XMFLOAT3 direction;
	};

}
