#include "PMXMaterial.h"

using namespace PMX;

RenderMaterial::RenderMaterial()
{
	baseTexture.reset();
	sphereTexture.reset();
	toonTexture.reset();

	dirty = DirtyFlags::Clean;

	indexCount = 0;
	materialIndex = 0;
	startIndex = 0;

	weight = 0.0f;

	initializeAdditiveMaterialMorph(additive);
	initializeMultiplicativeMaterialMorph(multiplicative);
}

RenderMaterial::~RenderMaterial()
{
	Shutdown();
}

void RenderMaterial::Shutdown()
{
	baseTexture.reset();
	sphereTexture.reset();
	toonTexture.reset();
}

DirectX::XMFLOAT4 RenderMaterial::getDiffuse(Material *m)
{
	return DirectX::XMFLOAT4(m->diffuse.red * getMultiplicativeMorph()->diffuse.red + getAdditiveMorph()->diffuse.red,
		m->diffuse.green * getMultiplicativeMorph()->diffuse.green + getAdditiveMorph()->diffuse.green,
		m->diffuse.blue * getMultiplicativeMorph()->diffuse.blue + getAdditiveMorph()->diffuse.blue,
		m->diffuse.alpha * getMultiplicativeMorph()->diffuse.alpha + getAdditiveMorph()->diffuse.alpha);
}

DirectX::XMFLOAT4 RenderMaterial::getAmbient(Material *m)
{
	return DirectX::XMFLOAT4(m->ambient.red * getMultiplicativeMorph()->ambient.red + getAdditiveMorph()->ambient.red,
		m->ambient.green * getMultiplicativeMorph()->ambient.green + getAdditiveMorph()->ambient.green,
		m->ambient.blue * getMultiplicativeMorph()->ambient.blue + getAdditiveMorph()->ambient.blue,
		1.0f);
}

DirectX::XMFLOAT4 RenderMaterial::getAverage(Material *m)
{
	DirectX::XMFLOAT4 d = getDiffuse(m), a = getAmbient(m);

	return DirectX::XMFLOAT4((d.x + a.x) / 2.0f, (d.y + a.y) / 2.0f, (d.z + a.z) / 2.0f, (d.w + a.w) / 2.0f);
}

DirectX::XMFLOAT4 RenderMaterial::getSpecular(Material *m)
{
	return DirectX::XMFLOAT4(m->specular.red * getMultiplicativeMorph()->specular.red + getAdditiveMorph()->specular.red,
		m->specular.green * getMultiplicativeMorph()->specular.green + getAdditiveMorph()->specular.green,
		m->specular.blue * getMultiplicativeMorph()->specular.blue + getAdditiveMorph()->specular.blue,
		1.0f);
}

float RenderMaterial::getSpecularCoefficient(Material *m)
{
	return m->specularCoefficient * getMultiplicativeMorph()->specularCoefficient + getAdditiveMorph()->specularCoefficient;
}

MaterialMorph* RenderMaterial::getAdditiveMorph()
{
	if ((dirty & DirtyFlags::AdditiveMorph) == 0)
		return &additive;

	initializeAdditiveMaterialMorph(additive);
	for (auto &i : appliedMorphs)
	{
		if (i.first.method == MaterialMorphMethod::Additive)
			additive += i.first * i.second;
	}

	// Remove dirty flag
	dirty &= ~DirtyFlags::AdditiveMorph;

	return &additive;
}

MaterialMorph* RenderMaterial::getMultiplicativeMorph()
{
	if ((dirty & DirtyFlags::MultiplicativeMorph) == 0)
		return &multiplicative;

	initializeMultiplicativeMaterialMorph(multiplicative);

	weight = 0.0f;

	for (auto &i : appliedMorphs)
	{
		weight += i.second;
		if (i.first.method == MaterialMorphMethod::Multiplicative)
			multiplicative *= i.first * i.second;
	}

	weight /= appliedMorphs.size();

	dirty &= ~DirtyFlags::MultiplicativeMorph;

	return &multiplicative;
}

void RenderMaterial::ApplyMorph(MaterialMorph *morph, float weight)
{
	auto i = ([this, morph](){ for (auto i = appliedMorphs.begin(); i != appliedMorphs.end(); i++) { if (&i->first == morph) return i; } return appliedMorphs.end(); })();
	if (weight == 0.0f) {
		if (i != appliedMorphs.end())
			appliedMorphs.erase(i);
		else return;
	}
	else {
		if (i == appliedMorphs.end())
			appliedMorphs.push_back(std::make_pair(*morph, weight));
		else
			i->second = weight;
	}

	this->weight = 0.0f;
	for (auto &m : appliedMorphs) this->weight += m.second / appliedMorphs.size();

	if (morph->method == MaterialMorphMethod::Additive)
		dirty |= DirtyFlags::AdditiveMorph;
	else dirty |= DirtyFlags::MultiplicativeMorph;

	dirty |= DirtyFlags::Textures;
}

void RenderMaterial::initializeAdditiveMaterialMorph(MaterialMorph &morph)
{
	morph.ambient.red = morph.ambient.green = morph.ambient.blue = 0.0f;
	morph.diffuse.red = morph.diffuse.green = morph.diffuse.blue = morph.diffuse.alpha = 0.0f;
	morph.edgeColor.red = morph.edgeColor.green = morph.edgeColor.blue = morph.edgeColor.alpha = 0.0f;
	morph.edgeSize = 0.0f;
	morph.index = 0;
	morph.method = MaterialMorphMethod::Additive;
	morph.specular.red = morph.specular.green = morph.specular.blue = 0.0f;
	morph.specularCoefficient = 0.0f;

	morph.baseCoefficient.red = morph.baseCoefficient.green = morph.baseCoefficient.blue = morph.baseCoefficient.alpha = 0.0f;
	morph.sphereCoefficient.red = morph.sphereCoefficient.green = morph.sphereCoefficient.blue = morph.sphereCoefficient.alpha = 0.0f;
	morph.toonCoefficient.red = morph.toonCoefficient.green = morph.toonCoefficient.blue = morph.toonCoefficient.alpha = 0.0f;
}

void RenderMaterial::initializeMultiplicativeMaterialMorph(MaterialMorph &morph)
{
	morph.ambient.red = morph.ambient.green = morph.ambient.blue = 1.0f;
	morph.diffuse.red = morph.diffuse.green = morph.diffuse.blue = morph.diffuse.alpha = 1.0f;
	morph.edgeColor.red = morph.edgeColor.green = morph.edgeColor.blue = morph.edgeColor.alpha = 1.0f;
	morph.edgeSize = 1.0f;
	morph.index = 0;
	morph.method = MaterialMorphMethod::Multiplicative;
	morph.specular.red = morph.specular.green = morph.specular.blue = 1.0f;
	morph.specularCoefficient = 1.0f;

	morph.baseCoefficient.red = morph.baseCoefficient.green = morph.baseCoefficient.blue = morph.baseCoefficient.alpha = 1.0f;
	morph.sphereCoefficient.red = morph.sphereCoefficient.green = morph.sphereCoefficient.blue = morph.sphereCoefficient.alpha = 1.0f;
	morph.toonCoefficient.red = morph.toonCoefficient.green = morph.toonCoefficient.blue = morph.toonCoefficient.alpha = 1.0f;
}

MaterialMorph operator* (const MaterialMorph& morph, float weight)
{
	MaterialMorph m = morph;

	m.ambient *= weight;
	m.diffuse *= weight;
	m.diffuse.alpha = morph.diffuse.alpha;
	m.edgeColor *= weight;
	m.edgeSize *= weight;
	m.specular *= weight;
	m.specularCoefficient *= weight;
	m.baseCoefficient *= weight;
	m.baseCoefficient.alpha = morph.baseCoefficient.alpha;
	m.sphereCoefficient *= weight;
	m.sphereCoefficient.alpha = morph.sphereCoefficient.alpha;
	m.toonCoefficient *= weight;
	m.toonCoefficient.alpha = morph.toonCoefficient.alpha;

	return m;
}