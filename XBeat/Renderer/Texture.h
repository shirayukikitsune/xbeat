#pragma once

#include "DXUtil.h"
#include <string>
#include <list>

namespace Renderer {

class Texture
{
public:
	Texture();
	~Texture();

	bool Initialize(ID3D11Device *device, const std::wstring &file);
	void Shutdown();

	DXType<ID3D11ShaderResourceView> GetTexture();

private:
	DXType<ID3D11ShaderResourceView> texture;
};

}