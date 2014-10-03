#pragma once

#include "PMXDefinitions.h"

#include <list>

namespace PMX {

class RenderMaterial
{
public:
	struct DirtyFlags {
		enum Flags {
			Clean,
			VertexBuffer = 0x1,
			Textures = 0x2,
			AdditiveMorph = 0x4,
			MultiplicativeMorph = 0x8
		};
	};
	RenderMaterial();
	~RenderMaterial();

	void Shutdown();
	DirectX::XMFLOAT4 getDiffuse(Material *m);
	DirectX::XMFLOAT4 getAmbient(Material *m);
	DirectX::XMFLOAT4 getAverage(Material *m);
	DirectX::XMFLOAT4 getSpecular(Material *m);
	float getSpecularCoefficient(Material *m);

	std::shared_ptr<Renderer::Texture> baseTexture;
	std::shared_ptr<Renderer::Texture> sphereTexture;
	std::shared_ptr<Renderer::Texture> toonTexture;

	uint32_t materialIndex, startIndex;
	int indexCount;
	uint32_t dirty;
	DirectX::XMFLOAT4 avgColor;

	MaterialMorph* getAdditiveMorph();
	MaterialMorph* getMultiplicativeMorph();

	void ApplyMorph(MaterialMorph *morph, float weight);
	__forceinline float getWeight() { return weight; }

	static void initializeAdditiveMaterialMorph(MaterialMorph &morph);
	static void initializeMultiplicativeMaterialMorph(MaterialMorph &morph);

private:
	std::list<std::pair<MaterialMorph, float>> appliedMorphs;

	MaterialMorph additive, multiplicative;
	float weight;
};
}

PMX::MaterialMorph operator* (const PMX::MaterialMorph& morph, float weight);
