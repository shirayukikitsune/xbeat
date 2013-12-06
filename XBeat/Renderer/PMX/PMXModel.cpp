#include "../CameraClass.h"
#include "../Shaders/LightShader.h"
#include "../Light.h"
#include "PMXModel.h"
#include "PMXBone.h"
#include "PMXMaterial.h"
#include "../D3DRenderer.h"

#include <fstream>
#include <cstring>
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
	bodies.resize(0);

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
	vertices.resize(0);

	verticesIndex.resize(0);
	
	textures.resize(0);

	for (std::vector<PMX::Material*>::size_type i = 0; i < materials.size(); i++) {
		delete materials[i];
		materials[i] = nullptr;
	}
	materials.resize(0);

	for (std::vector<PMX::Bone*>::size_type i = 0; i < bones.size(); i++) {
		delete bones[i];
		bones[i] = nullptr;
	}
	bones.resize(0);

	for (std::vector<PMX::Morph*>::size_type i = 0; i < morphs.size(); i++) {
		delete morphs[i];
		morphs[i] = nullptr;
	}
	morphs.resize(0);

	for (std::vector<PMX::Frame*>::size_type i = 0; i < frames.size(); i++) {
		delete frames[i];
		frames[i] = nullptr;
	}
	frames.resize(0);

	for (std::vector<PMX::RigidBody*>::size_type i = 0; i < bodies.size(); i++) {
		delete bodies[i];
		bodies[i] = nullptr;
	}
	bodies.resize(0);

	for (std::vector<PMX::Joint*>::size_type i = 0; i < joints.size(); i++) {
		delete joints[i];
		joints[i] = nullptr;
	}
	joints.resize(0);

	for (std::vector<PMX::Joint*>::size_type i = 0; i < softBodies.size(); i++) {
		delete softBodies[i];
		softBodies[i] = nullptr;
	}
	softBodies.resize(0);
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
	D3D11_SUBRESOURCE_DATA vertexData, indexData, materialData;
	HRESULT result;
	// Used to generate the view box
	DirectX::XMFLOAT3 minPos(FLT_MAX, FLT_MAX, FLT_MAX), maxPos(FLT_MIN, FLT_MIN, FLT_MIN);

	materialBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	materialBufferDesc.ByteWidth = sizeof (Shaders::Light::MaterialBufferType) * this->materials.size();
	materialBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	materialBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	materialBufferDesc.MiscFlags = 0;
	materialBufferDesc.StructureByteStride = 0;

	this->rendermaterials.resize(this->materials.size());
	m_materialBufferData.resize(this->materials.size());

	uint32_t lastIndex = 0;

	for (uint32_t k = 0; k < this->rendermaterials.size(); k++) {
		std::shared_ptr<DirectX::VertexPositionNormalTexture> v(new DirectX::VertexPositionNormalTexture[this->materials[k]->indexCount]);
		if (v == nullptr)
			return false;

		std::shared_ptr<UINT> idx(new UINT[this->materials[k]->indexCount]);
		if (idx == nullptr)
			return false;

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
			v.get()[i].position = DirectX::XMFLOAT3(vertex->position.x(), vertex->position.y(), vertex->position.z());
			v.get()[i].normal = DirectX::XMFLOAT3(vertex->normal.x(), vertex->normal.y(), vertex->normal.z());
			v.get()[i].textureCoordinate = DirectX::XMFLOAT2(vertex->uv[0], vertex->uv[1]);
#endif

			if (vertex->position.x() < minPos.x) minPos.x = vertex->position.x();
			if (vertex->position.x() > maxPos.x) maxPos.x = vertex->position.x();
			if (vertex->position.y() < minPos.y) minPos.y = vertex->position.y();
			if (vertex->position.y() > maxPos.y) maxPos.y = vertex->position.y();
			if (vertex->position.z() < minPos.z) minPos.z = vertex->position.z();
			if (vertex->position.z() > maxPos.z) maxPos.z = vertex->position.z();

			idx.get()[i] =  i;
		}

		vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexBufferDesc.ByteWidth = sizeof (DirectX::VertexPositionNormalTexture) * this->materials[k]->indexCount;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		vertexData.pSysMem = v.get();
		vertexData.SysMemPitch = 0;
		vertexData.SysMemSlicePitch = 0;

		result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &this->rendermaterials[k].vertexBuffer);
		if (FAILED(result))
			return false;

		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof (UINT) * this->materials[k]->indexCount;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		indexData.pSysMem = idx.get();
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		lastIndex += this->materials[k]->indexCount;

		rendermaterials[k].dirty |= RenderMaterial::DirtyFlags::Textures;
		rendermaterials[k].materialIndex = k;
		rendermaterials[k].indexCount = this->materials[k]->indexCount;
		rendermaterials[k].avgColor = DirectX::XMFLOAT4(this->materials[k]->ambient.red+this->materials[k]->diffuse.red, this->materials[k]->ambient.green + this->materials[k]->diffuse.green, this->materials[k]->ambient.blue + this->materials[k]->diffuse.blue, this->materials[k]->diffuse.alpha);
		rendermaterials[k].center.x = (maxPos.x - minPos.x) / 2.0f;
		rendermaterials[k].center.y = (maxPos.y - minPos.y) / 2.0f;
		rendermaterials[k].center.z = (maxPos.z - maxPos.z) / 2.0f;
		rendermaterials[k].radius.x = abs(maxPos.x - rendermaterials[k].center.x);
		rendermaterials[k].radius.y = abs(maxPos.y - rendermaterials[k].center.y);
		rendermaterials[k].radius.z = abs(maxPos.z - rendermaterials[k].center.z);

		m_materialBufferData[k].ambientColor = rendermaterials[k].getAmbient(materials[k]);
		m_materialBufferData[k].diffuseColor = rendermaterials[k].getDiffuse(materials[k]);
		m_materialBufferData[k].flags = 0;
		m_materialBufferData[k].specularColor = rendermaterials[k].getSpecular(materials[k]);
		m_materialBufferData[k].addBaseCoefficient = color4ToFloat4(rendermaterials[k].getAdditiveMorph()->baseCoefficient);
		m_materialBufferData[k].addSphereCoefficient = color4ToFloat4(rendermaterials[k].getAdditiveMorph()->sphereCoefficient);
		m_materialBufferData[k].addToonCoefficient = color4ToFloat4(rendermaterials[k].getAdditiveMorph()->toonCoefficient);
		m_materialBufferData[k].mulBaseCoefficient = color4ToFloat4(rendermaterials[k].getMultiplicativeMorph()->baseCoefficient);
		m_materialBufferData[k].mulSphereCoefficient = color4ToFloat4(rendermaterials[k].getMultiplicativeMorph()->sphereCoefficient);
		m_materialBufferData[k].mulToonCoefficient = color4ToFloat4(rendermaterials[k].getMultiplicativeMorph()->toonCoefficient);

		result = device->CreateBuffer(&indexBufferDesc, &indexData, &this->rendermaterials[k].indexBuffer);
		if(FAILED(result))
			return false;
	}

	materialData.pSysMem = m_materialBufferData.data();
	materialData.SysMemPitch = 0;
	materialData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&materialBufferDesc, &materialData, &m_materialBuffer);
	if (FAILED(result))
		return false;

	m_dirtyBuffer = true;

	// Initialize bone buffers
	for (auto &bone : bones) {
		bone->Initialize(d3d);
	}

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

bool PMX::Model::updateMaterialBuffer(DXType<ID3D11DeviceContext> context)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr;
	ID3D11ShaderResourceView *textures[3];
	Shaders::Light::MaterialBufferType *matBuffer;

	if (!m_dirtyBuffer)
		return true;

	m_dirtyBuffer = false;

	hr = context->Map(m_materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(hr))
		return false;

	matBuffer = (Shaders::Light::MaterialBufferType*)mappedResource.pData;

	for (uint32_t i = 0; i < rendermaterials.size(); i++) {
		if (rendermaterials[i].dirty & RenderMaterial::DirtyFlags::Textures) {
			textures[0] = rendermaterials[i].baseTexture ? rendermaterials[i].baseTexture->GetTexture() : nullptr;
			textures[1] = rendermaterials[i].sphereTexture ? rendermaterials[i].sphereTexture->GetTexture() : nullptr;
			textures[2] = rendermaterials[i].toonTexture ? rendermaterials[i].toonTexture->GetTexture() : nullptr;

			matBuffer[i].flags = 0;

			matBuffer[i].flags |= (textures[1] == nullptr || materials[i]->sphereMode == MaterialSphereMode::Disabled) ? 0x01 : 0;
			matBuffer[i].flags |= materials[i]->sphereMode == MaterialSphereMode::Add ? 0x02 : 0;
			matBuffer[i].flags |= textures[2] == nullptr ? 0x04 : 0;
			matBuffer[i].flags |= textures[0] == nullptr ? 0x08 : 0;

			matBuffer[i].morphWeight = rendermaterials[i].getWeight();

			matBuffer[i].addBaseCoefficient = color4ToFloat4(rendermaterials[i].getAdditiveMorph()->baseCoefficient);
			matBuffer[i].addSphereCoefficient = color4ToFloat4(rendermaterials[i].getAdditiveMorph()->sphereCoefficient);
			matBuffer[i].addToonCoefficient = color4ToFloat4(rendermaterials[i].getAdditiveMorph()->toonCoefficient);
			matBuffer[i].mulBaseCoefficient = color4ToFloat4(rendermaterials[i].getMultiplicativeMorph()->baseCoefficient);
			matBuffer[i].mulSphereCoefficient = color4ToFloat4(rendermaterials[i].getMultiplicativeMorph()->sphereCoefficient);
			matBuffer[i].mulToonCoefficient = color4ToFloat4(rendermaterials[i].getMultiplicativeMorph()->toonCoefficient);

			matBuffer[i].specularColor = rendermaterials[i].getSpecular(materials[i]);

			if ((matBuffer[i].flags & 0x04) == 0) {
				matBuffer[i].diffuseColor = rendermaterials[i].getDiffuse(materials[i]);
				matBuffer[i].ambientColor = rendermaterials[i].getAmbient(materials[i]);
			}
			else {
				matBuffer[i].diffuseColor = matBuffer[i].ambientColor = rendermaterials[i].getAverage(materials[i]);
			}

			rendermaterials[i].dirty &= ~RenderMaterial::DirtyFlags::Textures;
			m_materialBufferData[i] = matBuffer[i];
		}
	}

	context->Unmap(m_materialBuffer, 0);

	return true;
}

bool PMX::Model::RenderBuffers(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<CameraClass> camera, std::shared_ptr<ViewFrustum> frustum)
{
	if ((m_debugFlags & DebugFlags::DontRenderModel) == 0) {
		unsigned int stride;
		unsigned int offset;
		DirectX::XMFLOAT3 cameraPosition;
		ID3D11DeviceContext *context = d3d->GetDeviceContext();
		Shaders::Light::MaterialBufferType matBuf;
		DirectX::XMMATRIX wvp = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(world, view), projection);

		DirectX::XMStoreFloat3(&cameraPosition, camera->GetPosition());

		ID3D11ShaderResourceView *textures[3];

		int renderedMaterials = 0;

		if (!updateMaterialBuffer(context))
			return false;

		if (!lightShader->SetShaderParameters(context, world, wvp, light->GetDirection(), light->GetDiffuseColor(), light->GetAmbientColor(), cameraPosition, light->GetSpecularColor(), light->GetSpecularPower(), m_materialBuffer))
			return false;

		bool isTransparent = false;

		for (uint32_t i = 0; i < rendermaterials.size(); i++) {
			// If material is marked for update, then update it here
			if (rendermaterials[i].dirty & RenderMaterial::DirtyFlags::VertexBuffer)
				updateMaterialVertexBuffer(&rendermaterials[i]);

			// Check if we need to render this material
			if (!frustum->IsBoxInside(rendermaterials[i].center, rendermaterials[i].radius))
				continue; // Material is not inside the view frustum, so just skip

			renderedMaterials++;

			// Set vertex buffer stride and offset.
			stride = sizeof(DirectX::VertexPositionNormalTexture);
			offset = 0;

			textures[0] = rendermaterials[i].baseTexture ? rendermaterials[i].baseTexture->GetTexture() : nullptr;
			textures[1] = rendermaterials[i].sphereTexture ? rendermaterials[i].sphereTexture->GetTexture() : nullptr;
			textures[2] = rendermaterials[i].toonTexture ? rendermaterials[i].toonTexture->GetTexture() : nullptr;

			if (m_materialBufferData[i].diffuseColor.w <= 0.0f || m_materialBufferData[i].ambientColor.w <= 0.0f)
				continue;

			if ((materials[i]->flags & MaterialFlags::DoubleSide) == MaterialFlags::DoubleSide ||
				(m_materialBufferData[i].diffuseColor.w < 1.0f || m_materialBufferData[i].ambientColor.w < 1.0f)) {
				// Enable no-culling rasterizer if material has transparency or if it is marked to be rendered by both sides

				if (!isTransparent) {
					context->RSSetState(d3d->GetRasterState(1));
					isTransparent = true;
				}
			}
			else if (isTransparent) {
				isTransparent = false;
				context->RSSetState(d3d->GetRasterState(0));
			}

			// Set the vertex buffer to active in the input assembler so it can be rendered.
			context->IASetVertexBuffers(0, 1, &rendermaterials[i].vertexBuffer, &stride, &offset);

			// Set the index buffer to active in the input assembler so it can be rendered.
			context->IASetIndexBuffer(rendermaterials[i].indexBuffer, DXGI_FORMAT_R32_UINT, 0);

			// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//if (!lightShader->Render(context, rendermaterials[i].indexCount, world, view, projection, textures, 3, light->GetDirection(), light->GetDiffuseColor(), light->GetAmbientColor(), cameraPosition, light->GetSpecularColor(), light->GetSpecularPower(), m_materialBuffer, i))
			//return false;

			if (!lightShader->SetMaterialInfo(context, i))
				return false;

			lightShader->RenderShader(context,
				rendermaterials[i].indexCount,
				textures,
				3);
		}

		if (isTransparent)
			context->RSSetState(d3d->GetRasterState(0));
	}

	if (m_debugFlags & DebugFlags::RenderBones) {
		for (auto &bone : bones)
			bone->Render(d3d, world, view, projection);
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

		if (materials[i]->toonFlag == 0 && materials[i]->toonTexture.custom < textures.size()) {
			rendermaterials[i].toonTexture = renderTextures[materials[i]->toonTexture.custom];
		}
		else if (materials[i]->toonFlag == 1 && materials[i]->toonTexture.default < defaultToonTexCount) {
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

void PMX::Model::updateMaterialVertexBuffer(PMX::RenderMaterial *material)
{
	ID3D11Device *device;
	ID3D11DeviceContext *context;
	DirectX::VertexPositionNormalTexture *vertices;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	Vertex *vertex;
	HRESULT result;

	if ((material->dirty & RenderMaterial::DirtyFlags::VertexBuffer) == 0)
		return;

	material->dirty &= ~RenderMaterial::DirtyFlags::VertexBuffer;

	// get the d3d11 device and context from the vertex buffer
	material->vertexBuffer->GetDevice(&device);
	device->GetImmediateContext(&context);

	// get access to the buffer
	result = context->Map(material->vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return;

	vertices = (DirectX::VertexPositionNormalTexture*)mappedResource.pData;
	// Update vertex data
	for (int i = 0; i < material->indexCount; i++) {
		vertex = this->vertices[this->verticesIndex[i + material->startIndex]];
		DirectX::XMStoreFloat3(&vertices[i].position, vertex->GetFinalPosition().mVec128);
		btVector3 n = vertex->GetNormal();
		vertices[i].normal = DirectX::XMFLOAT3(n.x(), n.y(), n.z());
		vertices[i].textureCoordinate = DirectX::XMFLOAT2(vertex->uv[0], vertex->uv[1]);
	}

	// release the access to the buffer
	context->Unmap(material->vertexBuffer, 0);
}


