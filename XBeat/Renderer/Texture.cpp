#include "Texture.h"

#include <algorithm>
#include <cstring>

#include <DirectXTex.h>

using namespace Renderer;

Texture::Texture()
{
	texture = nullptr;
}


Texture::~Texture(void)
{
	Shutdown();
}

bool Texture::Initialize(ID3D11Device *device, const std::wstring &file)
{
	HRESULT result;

	auto pos = file.find_last_of(L".");
	std::wstring extension;
	if (pos != std::string::npos && pos + 1 < file.length()) {
		extension = file.substr(pos+1);
		// Transform the extension to upper case
		std::transform(extension.begin(), extension.end(), extension.begin(), [](const wchar_t& ch) { return toupper(ch); });
	}
	DirectX::ScratchImage image;
	DirectX::TexMetadata metaData;

	if (extension == L"DDS") {
		result = DirectX::GetMetadataFromDDSFile(file.c_str(), DirectX::DDS_FLAGS_NONE, metaData);
		if (FAILED(result))
			return false;

		result = DirectX::LoadFromDDSFile(file.c_str(), DirectX::DDS_FLAGS_NONE, &metaData, image);
		if (FAILED(result))
			return false;
	}
	else if (extension == L"TGA") {
		result = DirectX::GetMetadataFromTGAFile(file.c_str(), metaData);
		if (FAILED(result))
			return false;

		result = DirectX::LoadFromTGAFile(file.c_str(), &metaData, image);
		if (FAILED(result))
			return false;
	}
	else {
		result = DirectX::GetMetadataFromWICFile(file.c_str(), DirectX::WIC_FLAGS_NONE, metaData);
		if (FAILED(result))
			return false;

		result = DirectX::LoadFromWICFile(file.c_str(), DirectX::WIC_FLAGS_NONE, &metaData, image);
		if (FAILED(result))
			return false;
	}

	result = DirectX::CreateShaderResourceView(device, image.GetImages(), image.GetImageCount(), metaData, &texture);
	if (FAILED(result))
		return false;

	return true;
}

void Texture::Shutdown()
{
	DX_DELETEIF(texture);
}

ID3D11ShaderResourceView *Texture::GetTexture()
{
	return texture;
}
