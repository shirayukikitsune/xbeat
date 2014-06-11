#include "../Camera.h"
#include "../Shaders/LightShader.h"
#include "../Light.h"
#include "PMXModel.h"
#include "PMXBone.h"
#include "PMXMaterial.h"
#include "PMXShader.h"
#include "../D3DRenderer.h"

#include <fstream>
#include <cstring>
#include <algorithm>
#include <cfloat> // FLT_MIN, FLT_MAX

#include "../Model.h"

using namespace std;

using namespace Renderer;

std::vector<std::shared_ptr<Texture>> PMX::Model::sharedToonTextures(0);

//#define EXTENDED_READ

const wchar_t* defaultToonTexs[] = {
	L"./Data/Textures/Toon/toon01.bmp",
	L"./Data/Textures/Toon/toon02.bmp",
	L"./Data/Textures/Toon/toon03.bmp",
	L"./Data/Textures/Toon/toon04.bmp",
	L"./Data/Textures/Toon/toon05.bmp",
	L"./Data/Textures/Toon/toon06.bmp",
	L"./Data/Textures/Toon/toon07.bmp",
	L"./Data/Textures/Toon/toon08.bmp",
	L"./Data/Textures/Toon/toon09.bmp",
	L"./Data/Textures/Toon/toon10.bmp"
};
const int defaultToonTexCount = sizeof (defaultToonTexs) / sizeof (defaultToonTexs[0]);

PMX::Model::Model(void)
{
	m_debugFlags = DebugFlags::None;
	for (auto &i : m_vertexCountPerMethod) i = 0U;

	m_indexBuffer = m_vertexBuffer = m_materialBuffer = nullptr;
}


PMX::Model::~Model(void)
{
	Shutdown();
}

bool PMX::Model::LoadModel(const wstring &filename)
{
	if (!loadModel(filename))
		return false;

	basePath = filename.substr(0, filename.find_last_of(L"\\/") + 1);

	// Build the sorted vertices list
	m_sortedVertices = vertices;
	std::sort(m_sortedVertices.begin(), m_sortedVertices.end(), [](Vertex* a, Vertex* b) { return a->weightMethod != VertexWeightMethod::BDEF4 && a->weightMethod < b->weightMethod; });
	std::for_each(m_sortedVertices.begin(), m_sortedVertices.end(), [this](Vertex* v) { m_vertexCountPerMethod[(size_t)v->weightMethod]++; });

	// Build the physics bodies
	m_rigidBodies.resize(bodies.size());
	btVector3 size;
	for (uint32_t i = 0; i < bodies.size(); i++) {
		m_rigidBodies[i].reset(new RigidBody);
		m_rigidBodies[i]->Initialize(m_physics, this, bodies[i]);

		// Free some memory, since we won't use the loader structure anymore
		delete bodies[i];
		bodies[i] = nullptr;
	}

	// Clear our vector size;
	bodies.clear();
	bodies.shrink_to_fit();

	for (auto &body : softBodies)
	{
		body->Create(m_physics, this);
	}

	return true;
}

void PMX::Model::ReleaseModel()
{
	for (std::vector<PMX::Vertex*>::size_type i = 0; i < vertices.size(); i++) {
		delete vertices[i];
		vertices[i] = nullptr;
	}
	vertices.shrink_to_fit();

	verticesIndex.clear();
	verticesIndex.shrink_to_fit();
	
	textures.shrink_to_fit();

	for (std::vector<PMX::Material*>::size_type i = 0; i < materials.size(); i++) {
		delete materials[i];
		materials[i] = nullptr;
	}
	materials.shrink_to_fit();

	for (std::vector<PMX::Bone*>::size_type i = 0; i < bones.size(); i++) {
		delete bones[i];
		bones[i] = nullptr;
	}
	bones.shrink_to_fit();

	for (std::vector<PMX::Morph*>::size_type i = 0; i < morphs.size(); i++) {
		delete morphs[i];
		morphs[i] = nullptr;
	}
	morphs.shrink_to_fit();

	for (std::vector<PMX::Frame*>::size_type i = 0; i < frames.size(); i++) {
		delete frames[i];
		frames[i] = nullptr;
	}
	frames.shrink_to_fit();

	for (std::vector<PMX::RigidBody*>::size_type i = 0; i < bodies.size(); i++) {
		delete bodies[i];
		bodies[i] = nullptr;
	}
	bodies.shrink_to_fit();

	for (std::vector<PMX::Joint*>::size_type i = 0; i < joints.size(); i++) {
		delete joints[i];
		joints[i] = nullptr;
	}
	joints.shrink_to_fit();

	for (std::vector<PMX::Joint*>::size_type i = 0; i < softBodies.size(); i++) {
		delete softBodies[i];
		softBodies[i] = nullptr;
	}
	softBodies.shrink_to_fit();

	m_vertices.clear();
	m_vertices.shrink_to_fit();
}

DirectX::XMFLOAT4 color4ToFloat4(const PMX::Color4 &c) 
{
	return DirectX::XMFLOAT4(c.red, c.green, c.blue, c.alpha);
}

bool PMX::Model::InitializeBuffers(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	if (d3d == nullptr)
		return true; // Exit silently...?

	m_d3d = d3d;

	ID3D11Device *device = d3d->GetDevice();

	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc, materialBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;

	materialBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	materialBufferDesc.ByteWidth = sizeof (Shaders::Light::MaterialBufferType);
	materialBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	materialBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	materialBufferDesc.MiscFlags = 0;
	materialBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&materialBufferDesc, NULL, &m_materialBuffer);
	if (FAILED(result))
		return false;

	this->rendermaterials.resize(this->materials.size());

	uint32_t lastIndex = 0;

	std::vector<UINT> idx;
	DirectX::XMUINT4 boneWeights, boneIndices;

	for (uint32_t k = 0; k < this->rendermaterials.size(); k++) {
		rendermaterials[k].startIndex = lastIndex;

		for (uint32_t i = 0; i < (uint32_t)this->materials[k]->indexCount; i++) {
			Vertex* vertex = vertices[this->verticesIndex[i + lastIndex]];
			vertex->materials.push_front(std::pair<RenderMaterial*,UINT>(&rendermaterials[k], i));
#if 0
			v.get()[i].position = DirectX::XMFLOAT3(vertex->position.x(), vertex->position.y(), vertex->position.z());
			v.get()[i].texture = DirectX::XMFLOAT2(vertex->uv[0], vertex->uv[1]);
			v.get()[i].normal = DirectX::XMFLOAT3(vertex->normal.x(), vertex->normal.y(), vertex->normal.z());
			/*v.get()[i].UV1 = DirectX::XMFLOAT4(vertex->uvEx[0].x(), vertex->uvEx[0].y(), vertex->uvEx[0].z(), vertex->uvEx[0].w());
			v.get()[i].UV2 = DirectX::XMFLOAT4(vertex->uvEx[1].x(), vertex->uvEx[1].y(), vertex->uvEx[1].z(), vertex->uvEx[1].w());
			v.get()[i].UV3 = DirectX::XMFLOAT4(vertex->uvEx[2].x(), vertex->uvEx[2].y(), vertex->uvEx[2].z(), vertex->uvEx[2].w());
			v.get()[i].UV4 = DirectX::XMFLOAT4(vertex->uvEx[3].x(), vertex->uvEx[3].y(), vertex->uvEx[3].z(), vertex->uvEx[3].w());*/
#else
			switch (vertex->weightMethod) {
			case VertexWeightMethod::BDEF1:
				boneWeights = DirectX::XMUINT4(vertex->boneInfo.BDEF.weights[0], 0, 0, 0);
				boneIndices = DirectX::XMUINT4(vertex->boneInfo.BDEF.boneIndexes[0], 0, 0, 0);
				break;
			case VertexWeightMethod::BDEF2:
				boneWeights = DirectX::XMUINT4(vertex->boneInfo.BDEF.weights[0], vertex->boneInfo.BDEF.weights[1], 0, 0);
				boneIndices = DirectX::XMUINT4(vertex->boneInfo.BDEF.boneIndexes[0], vertex->boneInfo.BDEF.boneIndexes[1], 0, 0);
				break;
			case VertexWeightMethod::QDEF:
			case VertexWeightMethod::BDEF4:
				boneWeights = DirectX::XMUINT4(vertex->boneInfo.BDEF.weights[0], vertex->boneInfo.BDEF.weights[1], vertex->boneInfo.BDEF.weights[2], vertex->boneInfo.BDEF.weights[3]);
				boneIndices = DirectX::XMUINT4(vertex->boneInfo.BDEF.boneIndexes[0], vertex->boneInfo.BDEF.boneIndexes[1], vertex->boneInfo.BDEF.boneIndexes[2], vertex->boneInfo.BDEF.boneIndexes[3]);
				break;
			case VertexWeightMethod::SDEF:
				boneWeights = DirectX::XMUINT4(vertex->boneInfo.SDEF.weightBias, 1.0f - vertex->boneInfo.SDEF.weightBias, 0, 0);
				boneIndices = DirectX::XMUINT4(vertex->boneInfo.SDEF.boneIndexes[0], vertex->boneInfo.SDEF.boneIndexes[1], 0, 0);
				break;
			}
			m_vertices.emplace_back(PMXShader::VertexType{
				DirectX::XMFLOAT3(vertex->position.x(), vertex->position.y(), vertex->position.z()),
				DirectX::XMFLOAT3(vertex->normal.x(), vertex->normal.y(), vertex->normal.z()),
				DirectX::XMFLOAT2(vertex->uv[0], vertex->uv[1]),
				/*{
					vertex->uvEx[0].get128(),
					vertex->uvEx[1].get128(),
					vertex->uvEx[2].get128(),
					vertex->uvEx[3].get128()
				},*/
				boneIndices,
				boneWeights,
				k,
			});
#endif

			idx.emplace_back(i);
		}

		lastIndex += this->materials[k]->indexCount;

		rendermaterials[k].dirty |= RenderMaterial::DirtyFlags::Textures;
		rendermaterials[k].materialIndex = k;
		rendermaterials[k].indexCount = this->materials[k]->indexCount;
		rendermaterials[k].avgColor = DirectX::XMFLOAT4(this->materials[k]->ambient.red+this->materials[k]->diffuse.red, this->materials[k]->ambient.green + this->materials[k]->diffuse.green, this->materials[k]->ambient.blue + this->materials[k]->diffuse.blue, this->materials[k]->diffuse.alpha);
	}

	// Initialize bone buffers
	for (auto &bone : bones) {
		bone->Initialize(d3d);
	}

	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = (UINT)(sizeof(PMXShader::VertexType) * m_vertices.size());
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = m_vertices.data();
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
		return false;

	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.ByteWidth = (UINT)(sizeof(UINT) * idx.size());
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = idx.data();
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
		return false;

#ifdef DEBUG
	m_vertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, 6, "PMX VB");
	m_indexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, 6, "PMX IB");
#endif

	return true;
}

void PMX::Model::ShutdownBuffers()
{
	if (m_materialBuffer) {
		m_materialBuffer->Release();
		m_materialBuffer = nullptr;
	}

	if (m_vertexBuffer) {
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}

	if (m_indexBuffer) {
		m_indexBuffer->Release();
		m_indexBuffer = nullptr;
	}

	for (auto &material : rendermaterials)
	{
		material.Shutdown();
	}

	rendermaterials.resize(0);
}

bool PMX::Model::loadModel(const wstring &filename)
{
	ifstream ifs;
	ifs.open(filename, ios::binary);
	if (!ifs.good())
		return false;

	bool ret = loadModel(ifs);
	ifs.close();

	return ret;
}

bool PMX::Model::loadModel(istream &ifs)
{
	Loader *loader = new Loader;
	try {
		if (!loader->FromStream(this, ifs)) {
			delete loader;
			return false;
		}
	}
	catch (std::exception &) {
		//cout << e.what() << endl;
		delete loader;
		return false;
	}

	// Lookup for the root bone
	rootBone = GetBoneById(frames[0]->morphs[0].id);

	return true;
}

bool PMX::Model::updateMaterialBuffer(uint32_t material, ID3D11DeviceContext *context)
{
	ID3D11ShaderResourceView *textures[3];
	auto shader = std::dynamic_pointer_cast<PMXShader>(m_shader);
	if (!shader) return false;

	auto &mbuffer = shader->GetMaterial(material);

	textures[0] = rendermaterials[material].baseTexture ? rendermaterials[material].baseTexture->GetTexture() : nullptr;
	textures[1] = rendermaterials[material].sphereTexture ? rendermaterials[material].sphereTexture->GetTexture() : nullptr;
	textures[2] = rendermaterials[material].toonTexture ? rendermaterials[material].toonTexture->GetTexture() : nullptr;

	mbuffer.flags = 0;

	mbuffer.flags |= (textures[1] == nullptr || materials[material]->sphereMode == MaterialSphereMode::Disabled) ? 0x01 : 0;
	mbuffer.flags |= materials[material]->sphereMode == MaterialSphereMode::Add ? 0x02 : 0;
	mbuffer.flags |= textures[2] == nullptr ? 0x04 : 0;
	mbuffer.flags |= textures[0] == nullptr ? 0x08 : 0;

	mbuffer.morphWeight = rendermaterials[material].getWeight();

	mbuffer.addBaseCoefficient = color4ToFloat4(rendermaterials[material].getAdditiveMorph()->baseCoefficient);
	mbuffer.addSphereCoefficient = color4ToFloat4(rendermaterials[material].getAdditiveMorph()->sphereCoefficient);
	mbuffer.addToonCoefficient = color4ToFloat4(rendermaterials[material].getAdditiveMorph()->toonCoefficient);
	mbuffer.mulBaseCoefficient = color4ToFloat4(rendermaterials[material].getMultiplicativeMorph()->baseCoefficient);
	mbuffer.mulSphereCoefficient = color4ToFloat4(rendermaterials[material].getMultiplicativeMorph()->sphereCoefficient);
	mbuffer.mulToonCoefficient = color4ToFloat4(rendermaterials[material].getMultiplicativeMorph()->toonCoefficient);

	mbuffer.specularColor = rendermaterials[material].getSpecular(materials[material]);

	if ((mbuffer.flags & 0x04) == 0) {
		mbuffer.diffuseColor = rendermaterials[material].getDiffuse(materials[material]);
		mbuffer.ambientColor = rendermaterials[material].getAmbient(materials[material]);
	}
	else {
		mbuffer.diffuseColor = mbuffer.ambientColor = rendermaterials[material].getAverage(materials[material]);
	}
	mbuffer.index = (int)material;

	return true;
}

void PMX::Model::Render(ID3D11DeviceContext *context, std::shared_ptr<ViewFrustum> frustum)
{
	if ((m_debugFlags & DebugFlags::DontUpdatePhysics) == 0) {
		for (auto &bone : bones) {
			if (bone->Update()) {
				bone->updateChildren();
			}
		}
	}

	if ((m_debugFlags & DebugFlags::DontRenderModel) == 0) {
		unsigned int stride = sizeof(PMXShader::VertexType);

		ID3D11ShaderResourceView *textures[3];

		unsigned int vsOffset = 0;
		// Set the vertex buffer to active in the input assembler so it can be rendered.
		context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &vsOffset);

		// Set the index buffer to active in the input assembler so it can be rendered.
		context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Check if our model was changed and perform the updates
		updateVertexBuffer();

		for (uint32_t i = 0; i < rendermaterials.size(); i++) {
			if (!updateMaterialBuffer(i, context))
				return;
		}

		auto shader = std::dynamic_pointer_cast<PMXShader>(m_shader);
		shader->UpdateMaterialBuffer(context);
		shader->PrepareRender(context);

		context->RSSetState(m_d3d->GetRasterState(1));
		m_d3d->EnableAlphaBlending();

		for (uint32_t i = 0; i < rendermaterials.size(); i++) {
			textures[0] = rendermaterials[i].baseTexture ? rendermaterials[i].baseTexture->GetTexture() : nullptr;
			textures[1] = rendermaterials[i].sphereTexture ? rendermaterials[i].sphereTexture->GetTexture() : nullptr;
			textures[2] = rendermaterials[i].toonTexture ? rendermaterials[i].toonTexture->GetTexture() : nullptr;

			context->PSSetShaderResources(0, 3, textures);
			m_shader->Render(context, rendermaterials[i].indexCount, rendermaterials[i].startIndex);
		}

		context->RSSetState(m_d3d->GetRasterState(0));
		m_d3d->DisableAlphaBlending();
	}

	if (m_debugFlags & DebugFlags::RenderBones) {
		for (auto &bone : bones)
			bone->Render(m_shader->GetCBuffer().matrix.world, m_shader->GetCBuffer().matrix.view, m_shader->GetCBuffer().matrix.projection);
	}
}

bool PMX::Model::LoadTexture(ID3D11Device *device)
{
	if (device == nullptr)
		return true; // Exit silently... ?

	// Initialize default toon textures, if they are not loaded
	if (sharedToonTextures.size() != defaultToonTexCount) {
		sharedToonTextures.resize(defaultToonTexCount);

		// load each default toon texture
		for (int i = 0; i < defaultToonTexCount; i++) {
			sharedToonTextures[i].reset(new Texture);

			if (!sharedToonTextures[i]->Initialize(device, defaultToonTexs[i]))
				return false;
		}
	}

	// Initialize each specificied texture
	renderTextures.resize(textures.size());
	for (uint32_t i = 0; i < renderTextures.size(); i++) {
		renderTextures[i].reset(new Texture);

		if (!renderTextures[i]->Initialize(device, basePath + textures[i])) {
			renderTextures[i].reset();
		}
	}

	// Assign the textures to each material
	for (uint32_t i = 0; i < rendermaterials.size(); i++) {
		bool hasSphere = materials[i]->sphereMode != MaterialSphereMode::Disabled;

		if (materials[i]->baseTexture < textures.size()) {
			rendermaterials[i].baseTexture = renderTextures[materials[i]->baseTexture];
		}

		if (hasSphere && materials[i]->sphereTexture < textures.size()) {
			rendermaterials[i].sphereTexture = renderTextures[materials[i]->sphereTexture];
		}

		if (materials[i]->toonFlag == MaterialToonMode::CustomTexture && materials[i]->toonTexture.custom < textures.size()) {
			rendermaterials[i].toonTexture = renderTextures[materials[i]->toonTexture.custom];
		}
		else if (materials[i]->toonFlag == MaterialToonMode::DefaultTexture && materials[i]->toonTexture.default < defaultToonTexCount) {
			rendermaterials[i].toonTexture = sharedToonTextures[materials[i]->toonTexture.default];			
		}
	}

	return true;
}

void PMX::Model::ReleaseTexture()
{
	renderTextures.clear();
}

PMX::Bone* PMX::Model::GetBoneByName(const std::wstring &JPname)
{
	for (auto bone : bones) {
		if (bone->GetName().japanese.compare(JPname) == 0)
			return bone;
	}

	return nullptr;
}

PMX::Bone* PMX::Model::GetBoneByENName(const std::wstring &ENname)
{
	for (auto bone : bones) {
		if (bone->GetName().english.compare(ENname) == 0)
			return bone;
	}

	return nullptr;
}

PMX::Bone* PMX::Model::GetBoneById(uint32_t id)
{
	if (id == -1)
		return rootBone;

	if (id >= bones.size())
		return nullptr;

	return bones[id];
}

PMX::Material* PMX::Model::GetMaterialById(uint32_t id)
{
	if (id == -1 || id >= materials.size())
		return nullptr;

	return materials[id];
}

PMX::RenderMaterial* PMX::Model::GetRenderMaterialById(uint32_t id)
{
	if (id == -1 || id >= rendermaterials.size())
		return nullptr;

	return &rendermaterials[id];
}

void PMX::Model::ApplyMorph(const std::wstring &nameJP, float weight)
{
	for (auto morph : morphs) {
		if (morph->name.japanese.compare(nameJP) == 0) {
			ApplyMorph(morph, weight);
			break;
		}
	}
}

void PMX::Model::ApplyMorph(Morph *morph, float weight)
{
	// 0.0 <= weight <= 1.0
	if (weight <= 0.0f)
		weight = 0.0f;
	else if (weight >= 1.0f)
		weight = 1.0f;

	// Do the work only if we have a different weight from before
	if (morph->appliedWeight == weight)
		return;

	morph->appliedWeight = weight;

	switch (morph->type) {
	case MorphType::Group:
		for (auto i : morph->data) {
			if (i.group.index < morphs.size() && i.group.index >= 0) {
				ApplyMorph(morphs[i.group.index], i.group.rate * weight);
			}
		}
		break;
	case MorphType::Vertex:
		applyVertexMorph(morph, weight);
		break;
	case MorphType::Bone:
		applyBoneMorph(morph, weight);
		break;
	case MorphType::Material:
		applyMaterialMorph(morph, weight);
		break;
	case MorphType::UV:
	case MorphType::UV1:
	case MorphType::UV2:
	case MorphType::UV3:
	case MorphType::UV4:
		break;
	case MorphType::Flip:
		applyFlipMorph(morph, weight);
		break;
	case MorphType::Impulse:
		break;
	}
}

void PMX::Model::applyVertexMorph(Morph *morph, float weight)
{
	for (auto i : morph->data) {
		Vertex *v = vertices[i.vertex.index];

		auto it = ([morph, v]() { for (auto i = v->morphs.begin(); i != v->morphs.end(); i++) if ((*i)->morph == morph) return i; return v->morphs.end(); })();
		if (it == v->morphs.end()) {
			if (weight == 0.0f)
				continue;
			else {
				Vertex::MorphData *md = new Vertex::MorphData;
				md->morph = morph; md->type = &i; md->weight = weight;
				v->morphs.push_back(md);
			}
		}
		else {
			if (weight == 0.0f)
				v->morphs.erase(it);
			else
				(*it)->weight = weight;
		}

		v->morphOffset.setZero();
		for (auto &m : v->morphs) {
			v->morphOffset.setX(v->morphOffset.x() + m->type->vertex.offset[0] * m->weight);
			v->morphOffset.setY(v->morphOffset.y() + m->type->vertex.offset[1] * m->weight);
			v->morphOffset.setZ(v->morphOffset.z() + m->type->vertex.offset[2] * m->weight);
		}

		// Mark the material for update next frame
		for (auto &m : v->materials)
			m.first->dirty |= RenderMaterial::DirtyFlags::VertexBuffer;
		//v->material->dirty |= RenderMaterial::DirtyFlags::VertexBuffer;
	}
}

void PMX::Model::applyBoneMorph(Morph *morph, float weight)
{
	for (auto i : morph->data) {
		Bone *bone = bones[i.bone.index];
		//bone->ApplyMorph(morph, weight);
		m_dispatcher->AddTask([bone, morph, weight]() { bone->ApplyMorph(morph, weight); });
	}
}

void PMX::Model::applyMaterialMorph(Morph *morph, float weight)
{
	for (auto i : morph->data) {
		// If target is -1, apply morph to all materials
		if (i.material.index == -1) {
			for (auto m : rendermaterials)
				applyMaterialMorph(&i, &m, weight);
		}
		else if (i.material.index < rendermaterials.size() && i.material.index >= 0) {
			applyMaterialMorph(&i, &rendermaterials[i.material.index], weight);
		}
	}
}

void PMX::Model::applyMaterialMorph(MorphType *morph, PMX::RenderMaterial *material, float weight)
{
	material->ApplyMorph(&morph->material, weight);

	m_dirtyBuffer = true;
}

void PMX::Model::applyFlipMorph(Morph* morph, float weight)
{
	// Flip morph is like a group morph, but it is only applied to a single index, while all the others are set to 0
	int index = (int)((morph->data.size() + 1) * weight) - 1;

	for (uint32_t i = 0; i < morph->data.size(); i++) {
		if (i == index)
			ApplyMorph(morphs[morph->data[i].group.index], morph->data[i].group.rate);
		else
			ApplyMorph(morphs[morph->data[i].group.index], 0.0f);
	}
}

void PMX::Model::updateVertexBuffer()
{
	ID3D11Device *device;
	ID3D11DeviceContext *context;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	Vertex *vertex;
	HRESULT result;

	// Prevent vertex update if there are no updates to be done
	bool update = false;
	for (auto &material : rendermaterials) {
		if ((material.dirty & RenderMaterial::DirtyFlags::VertexBuffer) != 0) {
			update = true;
			break;
		}
	}

	if (!update) return;

	// get the d3d11 device and context from the vertex buffer
	m_vertexBuffer->GetDevice(&device);
	device->GetImmediateContext(&context);

	// get access to the buffer
	result = context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return;

	uint32_t i = 0, c = 0;
	for (; i < m_vertexCountPerMethod[(size_t)VertexWeightMethod::BDEF1]; ++i) {
		vertex = m_sortedVertices[i];
		
		DirectX::XMStoreFloat3(&vertex->dxPos, (vertex->GetFinalPosition() + vertex->boneOffset[0]).get128());
		btTransform t;
		t.setRotation(vertex->boneRotation[0]);
		DirectX::XMStoreFloat3(&vertex->dxNormal, t(vertex->normal).normalized().get128());
		vertex->dxUV = DirectX::XMFLOAT2(vertex->uv[0], vertex->uv[1]);
	}
	c += i;
	for (i = 0; i < m_vertexCountPerMethod[(size_t)VertexWeightMethod::BDEF2] + m_vertexCountPerMethod[(size_t)VertexWeightMethod::SDEF] + m_vertexCountPerMethod[(size_t)VertexWeightMethod::QDEF]; ++i) {
		vertex = m_sortedVertices[i + c];
		DirectX::XMStoreFloat3(&vertex->dxPos, (vertex->GetFinalPosition() + vertex->boneOffset[0] + vertex->boneOffset[1]).get128());
		btTransform t;
		t.setRotation(vertex->boneRotation[0] * vertex->boneRotation[1]);
		DirectX::XMStoreFloat3(&vertex->dxNormal, t(vertex->normal).normalized().get128());
		vertex->dxUV = DirectX::XMFLOAT2(vertex->uv[0], vertex->uv[1]);
	}
	c += i;
	for (; i < m_vertexCountPerMethod[(size_t)VertexWeightMethod::QDEF]; ++i) {
		vertex = m_sortedVertices[i + c];
		DirectX::XMStoreFloat3(&vertex->dxPos, (vertex->GetFinalPosition() + vertex->boneOffset[0] + vertex->boneOffset[1] + vertex->boneOffset[2] + vertex->boneOffset[3]).get128());
		btTransform t;
		t.setRotation(vertex->boneRotation[0] * vertex->boneRotation[1] * vertex->boneRotation[2] * vertex->boneRotation[3]);
		DirectX::XMStoreFloat3(&vertex->dxNormal, t(vertex->normal).normalized().get128());
		vertex->dxUV = DirectX::XMFLOAT2(vertex->uv[0], vertex->uv[1]);
	}

	for (auto &material : rendermaterials) {
		if ((material.dirty & RenderMaterial::DirtyFlags::VertexBuffer) == 0) continue;

		material.dirty &= ~RenderMaterial::DirtyFlags::VertexBuffer;

		// Update vertex data
		for (uint32_t i = material.startIndex; i < material.indexCount + material.startIndex; i++) {
			vertex = this->vertices[this->verticesIndex[i]];
			m_vertices[i].position = vertex->dxPos;
			m_vertices[i].normal = vertex->dxNormal;
			m_vertices[i].uv = vertex->dxUV;
		}
	}

	memcpy(mappedResource.pData, m_vertices.data(), sizeof(PMXShader::VertexType) * m_vertices.size());

	// release the access to the buffer
	context->Unmap(m_vertexBuffer, 0);
}


