#pragma once

#include <DirectXMath.h>

namespace Renderer {

	class Light
	{
	public:
		Light();
		~Light();

		void SetAmbientColor(float r, float g, float b, float a);
		void SetDiffuseColor(float r, float g, float b, float a);
		void SetSpecularColor(float r, float g, float b, float a);
		void SetDirection(float x, float y, float z);
		void SetSpecularPower(float p);

		DirectX::XMFLOAT4 GetAmbientColor();
		DirectX::XMFLOAT4 GetDiffuseColor();
		DirectX::XMFLOAT4 GetSpecularColor();
		DirectX::XMFLOAT3 GetDirection();
		float GetSpecularPower();

	private:
		DirectX::XMFLOAT4 ambientColor;
		DirectX::XMFLOAT4 diffuseColor;
		DirectX::XMFLOAT4 specularColor;
		DirectX::XMFLOAT3 direction;
		float specularPower;
	};

}
