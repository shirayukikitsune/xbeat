#include "../CameraClass.h"
#include "../Shaders/LightShader.h"
#include "../Light.h"
#include "PMXModel.h"
#include "PMXBone.h"
#include "PMXMaterial.h"
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
			m_vertices.emplace_back(DirectX::VertexPositionNormalTexture(vertex->position.get128(), vertex->normal.get128(), btVector4(vertex->uv[0], vertex->uv[1], 0.0f, 0.0f).get128()));
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
	vertexBufferDesc.ByteWidth = sizeof(DirectX::VertexPositionNormalTexture) * m_vertices.size();
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
	indexBufferDesc.ByteWidth = sizeof(UINT) * idx.size();
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

	return true;
}

void PMX::Model::ShutdownBuffers()
{
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

bool PMX::Model::updateMaterialBuffer(uint32_t material, DXType<ID3D11DeviceContext> context)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr;
	ID3D11ShaderResourceView *textures[3];
	Shaders::Light::MaterialBufferType *matBuffer;

	hr = context->Map(m_materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(hr))
		return false;

	matBuffer = (Shaders::Light::MaterialBufferType*)mappedResource.pData;

	if (material < rendermaterials.size()) {
		textures[0] = rendermaterials[material].baseTexture ? rendermaterials[material].baseTexture->GetTexture() : nullptr;
		textures[1] = rendermaterials[material].sphereTexture ? rendermaterials[material].sphereTexture->GetTexture() : nullptr;
		textures[2] = rendermaterials[material].toonTexture ? rendermaterials[material].toonTexture->GetTexture() : nullptr;

		matBuffer->flags = 0;

		matBuffer->flags |= (textures[1] == nullptr || materials[material]->sphereMode == MaterialSphereMode::Disabled) ? 0x01 : 0;
		matBuffer->flags |= materials[material]->sphereMode == MaterialSphereMode::Add ? 0x02 : 0;
		matBuffer->flags |= textures[2] == nullptr ? 0x04 : 0;
		matBuffer->flags |= textures[0] == nullptr ? 0x08 : 0;

		matBuffer->morphWeight = rendermaterials[material].getWeight();

		matBuffer->addBaseCoefficient = color4ToFloat4(rendermaterials[material].getAdditiveMorph()->baseCoefficient);
		matBuffer->addSphereCoefficient = color4ToFloat4(rendermaterials[material].getAdditiveMorph()->sphereCoefficient);
		matBuffer->addToonCoefficient = color4ToFloat4(rendermaterials[material].getAdditiveMorph()->toonCoefficient);
		matBuffer->mulBaseCoefficient = color4ToFloat4(rendermaterials[material].getMultiplicativeMorph()->baseCoefficient);
		matBuffer->mulSphereCoefficient = color4ToFloat4(rendermaterials[material].getMultiplicativeMorph()->sphereCoefficient);
		matBuffer->mulToonCoefficient = color4ToFloat4(rendermaterials[material].getMultiplicativeMorph()->toonCoefficient);

		matBuffer->specularColor = rendermaterials[material].getSpecular(materials[material]);

		if ((matBuffer->flags & 0x04) == 0) {
			matBuffer->diffuseColor = rendermaterials[material].getDiffuse(materials[material]);
			matBuffer->ambientColor = rendermaterials[material].getAmbient(materials[material]);
		}
		else {
			matBuffer->diffuseColor = matBuffer->ambientColor = rendermaterials[material].getAverage(materials[material]);
		}
		matBuffer->index = (int)material;
	}
	else matBuffer->index = -1;

	context->Unmap(m_materialBuffer, 0);

	return true;
}

bool PMX::Model::RenderBuffers(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<CameraClass> camera, std::shared_ptr<ViewFrustum> frustum)
{
	if ((m_debugFlags & DebugFlags::DontUpdatePhysics) == 0) {
		for (auto &bone : bones) {
			if (bone->Update()) {
				bone->updateChildren();
			}
		}
	}

	if ((m_debugFlags & DebugFlags::DontRenderModel) == 0) {
		unsigned int stride = sizeof(DirectX::VertexPositionNormalTexture);
		DirectX::XMFLOAT3 cameraPosition;
		ID3D11DeviceContext *context = d3d->GetDeviceContext();
		Shaders::Light::MaterialBufferType matBuf;
		DirectX::XMMATRIX wvp = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(world, view), projection);

		DirectX::XMStoreFloat3(&cameraPosition, camera->GetPosition());

		ID3D11ShaderResourceView *textures[3];

		if (!lightShader->SetShaderParameters(context, world, wvp, light->GetDirection(), light->GetDiffuseColor(), light->GetAmbientColor(), cameraPosition, light->GetSpecularColor(), light->GetSpecularPower(), m_materialBuffer))
			return false;

		unsigned int vsOffset = 0;
		// Set the vertex buffer to active in the input assembler so it can be rendered.
		context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &vsOffset);

		// Set the index buffer to active in the input assembler so it can be rendered.
		context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		bool isTransparent = false;

		// Check if our model was changed and perform the updates
		updateVertexBuffer();

		for (uint32_t i = 0; i < rendermaterials.size(); i++) {
			if (!updateMaterialBuffer(i, context))
				return false;

			textures[0] = rendermaterials[i].baseTexture ? rendermaterials[i].baseTexture->GetTexture() : nullptr;
			textures[1] = rendermaterials[i].sphereTexture ? rendermaterials[i].sphereTexture->GetTexture() : nullptr;
			textures[2] = rendermaterials[i].toonTexture ? rendermaterials[i].toonTexture->GetTexture() : nullptr;

			if ((materials[i]->flags & (uint8_t)MaterialFlags::DoubleSide) == (uint8_t)MaterialFlags::DoubleSide ||
				(rendermaterials[i].getDiffuse(materials[i]).w < 1.0f || rendermaterials[i].getAmbient(materials[i]).w < 1.0f)) {
				// Enable no-culling rasterizer if material has transparency or if it is marked to be rendered by both sides

				if (!isTransparent) {
					context->RSSetState(d3d->GetRasterState(1));
					isTransparent = true;
					d3d->EnableAlphaBlending();
				}
			}
			else if (isTransparent) {
				isTransparent = false;
				context->RSSetState(d3d->GetRasterState(0));
				d3d->DisableAlphaBlending();
			}

			lightShader->RenderShader(context,
				rendermaterials[i].indexCount,
				rendermaterials[i].startIndex,
				textures,
				3);
		}

		if (isTransparent) {
			context->RSSetState(d3d->GetRasterState(0));
			d3d->DisableAlphaBlending();
		}
	}

	if (m_debugFlags & DebugFlags::RenderBones) {
		for (auto &bone : bones)
			bone->Render(world, view, projection);
	}

	return true;
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
	for (auto &tex : renderTextures) {
		if (tex != nullptr) {
			tex->Shutdown();
			tex.reset();
		}
	}

	renderTextures.resize(0);
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
		DirectX::XMStoreFloat3(&vertex->dxPos, (vertex->GetFinalPosition() + vertex->boneOffset[0]).get128());
		btTransform t;
		t.setRotation(vertex->boneRotation[0] * vertex->boneRotation[1]);
		DirectX::XMStoreFloat3(&vertex->dxNormal, t(vertex->normal).normalized().get128());
		vertex->dxUV = DirectX::XMFLOAT2(vertex->uv[0], vertex->uv[1]);
	}
	c += i;
	for (; i < m_vertexCountPerMethod[(size_t)VertexWeightMethod::QDEF]; ++i) {
		vertex = m_sortedVertices[i + c];
		DirectX::XMStoreFloat3(&vertex->dxPos, (vertex->GetFinalPosition() + vertex->boneOffset[0]).get128());
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
			m_vertices[i].textureCoordinate = vertex->dxUV;
		}
	}

	memcpy(mappedResource.pData, m_vertices.data(), sizeof(DirectX::VertexPositionNormalTexture) * m_vertices.size());

	// release the access to the buffer
	context->Unmap(m_vertexBuffer, 0);
}


