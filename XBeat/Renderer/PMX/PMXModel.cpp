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

	m_indexBuffer = m_vertexBuffer = m_materialBuffer = nullptr;

	rootBone = new Bone(this, -1);
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

	for (auto &body : softBodies)
	{
		body->Create(m_physics, this);
	}

	for (auto &bone : bones) {
		if (bone->HasAnyFlag(BoneFlags::AfterPhysicalDeformation))
			m_postPhysicsBones.push_back(bone);
		else
			m_prePhysicsBones.push_back(bone);
	}

	auto sortFn = [](Bone* a, Bone* b) {
		return a->GetDeformationOrder() < b->GetDeformationOrder() || (a->GetDeformationOrder() == b->GetDeformationOrder() && a->GetId() < b->GetId());
	};

	std::sort(m_prePhysicsBones.begin(), m_prePhysicsBones.end(), sortFn);
	std::sort(m_postPhysicsBones.begin(), m_postPhysicsBones.end(), sortFn);

	return true;
}

void PMX::Model::ReleaseModel()
{
	for (std::vector<PMX::Vertex*>::size_type i = 0; i < vertices.size(); i++) {
		delete vertices[i];
		vertices[i] = nullptr;
	}
	vertices.clear();
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
	m_prePhysicsBones.clear();
	m_postPhysicsBones.clear();

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

	for (auto &joint : m_joints) {
		joint->Shutdown(m_physics);
	}
	m_joints.clear();
	m_joints.shrink_to_fit();

	for (std::vector<PMX::SoftBody*>::size_type i = 0; i < softBodies.size(); i++) {
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
	DirectX::XMFLOAT4 boneWeights;
	DirectX::XMUINT4 boneIndices;

	for (uint32_t k = 0; k < this->rendermaterials.size(); k++) {
		rendermaterials[k].startIndex = lastIndex;

		for (uint32_t i = 0; i < (uint32_t)this->materials[k]->indexCount; i++) {
			Vertex* vertex = vertices[this->verticesIndex[i + lastIndex]];
			vertex->materials.push_front(std::pair<RenderMaterial*,UINT>(&rendermaterials[k], i));

			switch (vertex->weightMethod) {
			case VertexWeightMethod::BDEF1:
				boneWeights = DirectX::XMFLOAT4(vertex->boneInfo.BDEF.weights[0], 0, 0, 0);
				boneIndices = DirectX::XMUINT4(vertex->boneInfo.BDEF.boneIndexes[0], 0, 0, 0);
				break;
			case VertexWeightMethod::BDEF2:
				boneWeights = DirectX::XMFLOAT4(vertex->boneInfo.BDEF.weights[0], vertex->boneInfo.BDEF.weights[1], 0, 0);
				boneIndices = DirectX::XMUINT4(vertex->boneInfo.BDEF.boneIndexes[0], vertex->boneInfo.BDEF.boneIndexes[1], 0, 0);
				break;
			case VertexWeightMethod::QDEF:
			case VertexWeightMethod::BDEF4:
				boneWeights = DirectX::XMFLOAT4(vertex->boneInfo.BDEF.weights[0], vertex->boneInfo.BDEF.weights[1], vertex->boneInfo.BDEF.weights[2], vertex->boneInfo.BDEF.weights[3]);
				boneIndices = DirectX::XMUINT4(vertex->boneInfo.BDEF.boneIndexes[0], vertex->boneInfo.BDEF.boneIndexes[1], vertex->boneInfo.BDEF.boneIndexes[2], vertex->boneInfo.BDEF.boneIndexes[3]);
				break;
			case VertexWeightMethod::SDEF:
				boneWeights = DirectX::XMFLOAT4(vertex->boneInfo.SDEF.weightBias, 1.0f - vertex->boneInfo.SDEF.weightBias, 0, 0);
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

			idx.emplace_back(i);
		}

		lastIndex += this->materials[k]->indexCount;

		rendermaterials[k].dirty |= RenderMaterial::DirtyFlags::Textures;
		rendermaterials[k].materialIndex = k;
		rendermaterials[k].indexCount = this->materials[k]->indexCount;
		rendermaterials[k].avgColor = DirectX::XMFLOAT4(this->materials[k]->ambient.red+this->materials[k]->diffuse.red, this->materials[k]->ambient.green + this->materials[k]->diffuse.green, this->materials[k]->ambient.blue + this->materials[k]->diffuse.blue, this->materials[k]->diffuse.alpha);
	}

	// Initialize bone buffers
	rootBone->Initialize(d3d);
	for (auto &bone : bones) {
		bone->Initialize(d3d);
	}

	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.ByteWidth = (UINT)(sizeof(PMXShader::VertexType) * m_vertices.size());
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = m_vertices.data();
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
		return false;

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_VERTEX_BUFFER;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_tmpVertexBuffer);
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
	m_tmpVertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, 6, "PMX SO");
	m_vertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, 6, "PMX VB");
	m_indexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, 6, "PMX IB");
#endif

	// Build the physics bodies
	m_rigidBodies.resize(bodies.size());
	for (uint32_t i = 0; i < bodies.size(); i++) {
		m_rigidBodies[i].reset(new RigidBody);
		m_rigidBodies[i]->Initialize(d3d->GetDeviceContext(), m_physics, this, bodies[i]);

		// Free some memory, since we won't use the loader structure anymore
		delete bodies[i];
		bodies[i] = nullptr;
	}

	// Clear our vector size;
	bodies.clear();
	bodies.shrink_to_fit();

	// Add the joints
	m_joints.resize(joints.size());
	for (uint32_t i = 0; i < joints.size(); i++) {
		m_joints[i].reset(new Joint);
		if (!m_joints[i]->Initialize(m_physics, this, joints[i]))
			return false;

		delete joints[i];
		joints[i] = nullptr;
	}
	joints.clear();
	joints.shrink_to_fit();

	return true;
}

void PMX::Model::ShutdownBuffers()
{
	if (m_materialBuffer) {
		m_materialBuffer->Release();
		m_materialBuffer = nullptr;
	}

	if (m_tmpVertexBuffer) {
		m_tmpVertexBuffer->Release();
		m_tmpVertexBuffer = nullptr;
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

bool PMX::Model::Update(float msec)
{
	if ((m_debugFlags & DebugFlags::DontUpdatePhysics) == 0) {
		rootBone->Update();

		for (auto &bone : m_prePhysicsBones) {
			bone->Update();
		}

		for (auto &body : m_rigidBodies) {
			body->Update();
		}

		for (auto &bone : m_postPhysicsBones) {
			bone->Update();
		}
	}

	return true;
}

void PMX::Model::Render(ID3D11DeviceContext *context, std::shared_ptr<ViewFrustum> frustum)
{
	if ((m_debugFlags & DebugFlags::DontRenderModel) == 0) {
		unsigned int stride = sizeof(PMXShader::VertexType);

		ID3D11ShaderResourceView *textures[3];

		unsigned int vsOffset = 0;

		context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto shader = std::dynamic_pointer_cast<PMXShader>(m_shader);

		bool update = false;
		for (auto & bone : bones) {
			if (bone->wasTouched()) {
				auto &shaderBone = shader->GetBone(bone->GetId());
				DirectX::XMMATRIX t = DirectX::XMMatrixTranspose(bone->getLocalTransform());
				shaderBone.transform[0] = t.r[0];
				shaderBone.transform[1] = t.r[1];
				shaderBone.transform[2] = t.r[2];
				shaderBone.position = bone->GetInitialPosition();
				update = true;
			}
		}
		if (update) {
			context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &vsOffset);

			shader->UpdateBoneBuffer(context);

			UINT offset = 0;
			context->SOSetTargets(1, &m_tmpVertexBuffer, &offset);
			ID3D11RenderTargetView *rtv; ID3D11DepthStencilView *dsv;
			context->OMGetRenderTargets(1, &rtv, &dsv);
			context->OMSetRenderTargets(0, nullptr, nullptr);
			shader->RenderGeometry(context, m_vertices.size(), 0);
			ID3D11Buffer *b = nullptr;
			context->OMSetRenderTargets(1, &rtv, dsv);
			dsv->Release();
			rtv->Release();
			context->SOSetTargets(1, &b, &offset);
		}

		context->IASetVertexBuffers(0, 1, &m_tmpVertexBuffer, &stride, &vsOffset);

		for (uint32_t i = 0; i < rendermaterials.size(); i++) {
			if (!updateMaterialBuffer(i, context))
				return;
		}

		shader->UpdateMaterialBuffer(context);
		shader->PrepareRender(context);

		m_d3d->EnableAlphaBlending();
		
		for (uint32_t i = 0; i < rendermaterials.size(); i++) {
			textures[0] = rendermaterials[i].baseTexture ? rendermaterials[i].baseTexture->GetTexture() : nullptr;
			textures[1] = rendermaterials[i].sphereTexture ? rendermaterials[i].sphereTexture->GetTexture() : nullptr;
			textures[2] = rendermaterials[i].toonTexture ? rendermaterials[i].toonTexture->GetTexture() : nullptr;

			if ((materials[i]->flags & (uint8_t)MaterialFlags::DoubleSide) != 0 || rendermaterials[i].getAmbient(materials[i]).w < 1.0f) {
				context->RSSetState(m_d3d->GetRasterState(1));
			}
			else context->RSSetState(m_d3d->GetRasterState(0));

			context->PSSetShaderResources(0, 3, textures);
			m_shader->Render(context, rendermaterials[i].indexCount, rendermaterials[i].startIndex);
		}
		
		m_d3d->DisableAlphaBlending();
	}

	DirectX::XMMATRIX view = DirectX::XMMatrixTranspose(m_shader->GetCBuffer().matrix.view);
	DirectX::XMMATRIX projection = DirectX::XMMatrixTranspose(m_shader->GetCBuffer().matrix.projection);

	if (m_debugFlags & DebugFlags::RenderRigidBodies) {
		context->RSSetState(m_d3d->GetRasterState(1));

		for (auto &body : m_rigidBodies) {
			body->Render(rootBone->getLocalTransform(), view, projection);
		}

		context->RSSetState(m_d3d->GetRasterState(0));
	}

	if (m_debugFlags & DebugFlags::RenderBones) {
		context->RSSetState(m_d3d->GetRasterState(1));

		for (auto &bone : bones)
			bone->Render(rootBone->getLocalTransform(), view, projection);

		context->RSSetState(m_d3d->GetRasterState(0));
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

std::shared_ptr<PMX::RigidBody> PMX::Model::GetRigidBodyById(uint32_t id)
{
	if (id == -1 || id >= m_rigidBodies.size())
		return nullptr;

	return m_rigidBodies[id];
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

