#pragma once

#include <d3d11.h>
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

	ID3D11ShaderResourceView *GetTexture();

private:
	ID3D11ShaderResourceView *texture;
};

}