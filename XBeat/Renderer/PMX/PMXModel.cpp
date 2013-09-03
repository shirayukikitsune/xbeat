#include "../CameraClass.h"
#include "../Shaders/LightShader.h"
#include "../Light.h"
#include "PMXModel.h"
#include "../D3DRenderer.h"

#include <fstream>
#include <cstring>
#include <codecvt>
#include <cfloat> // FLT_MIN, FLT_MAX

#include "Model.h"

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
	header = new Header();
	sizeInfo = new SizeInfo();
}


PMX::Model::~Model(void)
{
	Shutdown();
}


template <class T>
T __getString(std::istream &is)
{
	// Read length
	uint32_t len = 0;
	is.read((char*)&len, sizeof (uint32_t));
	len /= sizeof (T::value_type);

	// Read the string itself
	T::pointer data = new T::value_type[len];
	is.read((char*)data, len * sizeof (T::value_type));
	T retval(data, len);
	delete[] data;
	return retval;
}

wstring PMX::Model::getString(istream &is) {
	static wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> conversor;

	switch (sizeInfo->cbEncoding) {
	case 0:
		return __getString<wstring>(is);
	case 1:
		return conversor.from_bytes(__getString<string>(is));
	}

	return L"";
}

uint32_t readAsU32(uint8_t size, std::istream &is)
{
	uint8_t u8val;
	uint16_t u16val;
	uint32_t u32val;

	switch (size) {
	case 1:
		is.read((char*)&u8val, sizeof (uint8_t));
		if (u8val == 0xFF) return 0xFFFFFFFF;
		return u8val;
	case 2:
		is.read((char*)&u16val, sizeof (uint16_t));
		if (u16val == 0xFFFF) return 0xFFFFFFFF;
		return u16val;
	case 4:
		is.read((char*)&u32val, sizeof (uint32_t));
		return u32val;
	}

	return 0;
}

bool PMX::Model::LoadModel(const wstring &filename)
{
	if (!loadModel(filename))
		return false;

	basePath = filename.substr(0, filename.find_last_of(L"\\/") + 1);

	return true;
}

void PMX::Model::ReleaseModel()
{
	if (header != nullptr) {
		delete header;
		header = nullptr;
	}
	if (sizeInfo != nullptr) {
		delete sizeInfo;
		sizeInfo = nullptr;
	}

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
}

bool PMX::Model::InitializeBuffers(ID3D11Device *device)
{
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	// Used to generate the view box
	DirectX::XMFLOAT3 minPos(FLT_MAX, FLT_MAX, FLT_MAX), maxPos(FLT_MIN, FLT_MIN, FLT_MIN);

	this->rendermaterials.resize(this->materials.size());

	uint32_t lastIndex = 0;

	for (uint32_t k = 0; k < this->rendermaterials.size(); k++) {
		std::shared_ptr<VertexType> v(new VertexType[this->materials[k]->indexCount]);
		if (v == nullptr)
			return false;

		std::shared_ptr<UINT> idx(new UINT[this->materials[k]->indexCount]);
		if (idx == nullptr)
			return false;

		rendermaterials[k].startIndex = lastIndex;

		for (uint32_t i = 0; i < this->materials[k]->indexCount; i++) {
			Vertex* vertex = vertices[this->verticesIndex[i + lastIndex]];
			vertex->material = &rendermaterials[k];
			v.get()[i].position = DirectX::XMFLOAT3(vertex->position.x, vertex->position.y, vertex->position.z);
			v.get()[i].texture = DirectX::XMFLOAT2(vertex->uv[0], vertex->uv[1]);
			v.get()[i].normal = DirectX::XMFLOAT3(vertex->normal.x, vertex->normal.y, vertex->normal.z);

			if (vertex->position.x < minPos.x) minPos.x = vertex->position.x;
			if (vertex->position.x > maxPos.x) maxPos.x = vertex->position.x;
			if (vertex->position.y < minPos.y) minPos.y = vertex->position.y;
			if (vertex->position.y > maxPos.y) maxPos.y = vertex->position.y;
			if (vertex->position.z < minPos.z) minPos.z = vertex->position.z;
			if (vertex->position.z > maxPos.z) maxPos.z = vertex->position.z;

			idx.get()[i] =  i;
		}

		vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexBufferDesc.ByteWidth = sizeof (VertexType) * this->materials[k]->indexCount;
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

		rendermaterials[k].materialIndex = k;
		rendermaterials[k].indexCount = this->materials[k]->indexCount;
		rendermaterials[k].avgColor = DirectX::XMFLOAT4(this->materials[k]->ambient.red+this->materials[k]->diffuse.red, this->materials[k]->ambient.green + this->materials[k]->diffuse.green, this->materials[k]->ambient.blue + this->materials[k]->diffuse.blue, this->materials[k]->diffuse.alpha);
		rendermaterials[k].center.x = (maxPos.x - minPos.x) / 2.0f;
		rendermaterials[k].center.y = (maxPos.y - minPos.y) / 2.0f;
		rendermaterials[k].center.z = (maxPos.z - maxPos.z) / 2.0f;
		rendermaterials[k].radius.x = abs(maxPos.x - rendermaterials[k].center.x);
		rendermaterials[k].radius.y = abs(maxPos.y - rendermaterials[k].center.y);
		rendermaterials[k].radius.z = abs(maxPos.z - rendermaterials[k].center.z);
		generateAdditiveMaterialMorph(rendermaterials[k].additive);
		generateMultiplicativeMaterialMorph(rendermaterials[k].multiplicative);

		result = device->CreateBuffer(&indexBufferDesc, &indexData, &this->rendermaterials[k].indexBuffer);
		if(FAILED(result))
			return false;
	}

	return true;
}

void PMX::Model::ShutdownBuffers()
{
	for (int i = 0; i < rendermaterials.size(); i++)
	{
		rendermaterials[i].Shutdown();
	}

	rendermaterials.resize(0);
}

bool PMX::Model::loadModel(const wstring &filename)
{
	ifstream ifs;
	ifs.open(filename, ios::binary);
	if (!ifs.good())
		return false;

	ifs.read((char*)header, sizeof(Header));

	if (strncmp("Pmx ", header->abMagic, 4) != 0 && strncmp("PMX ", header->abMagic, 4) != 0)
		return false;

	if (header->fVersion != 1.0f && header->fVersion != 2.0f && header->fVersion != 2.1f)
		return false;

	ifs.read((char*)sizeInfo, sizeof(SizeInfo));

	description.nameJP = getString(ifs);
	description.nameEN = getString(ifs);
	description.commentJP = getString(ifs);
	description.commentEN = getString(ifs);

	int count;
	ifs.read((char*)&count, sizeof (int));
	vertices.resize(count);

	int k;
	float *uvExData = new float[sizeInfo->cbUVVectorSize];
	for (int i = 0; i < count; i++) {
		vertices[i] = new Vertex();
		ifs.read((char*)vertices[i], sizeof (float) * 8);

		if (sizeInfo->cbUVVectorSize > 0) {
			ifs.read((char*)uvExData, sizeof (float) * sizeInfo->cbUVVectorSize);
			vertices[i]->uvEx.assign(uvExData, uvExData + sizeInfo->cbUVVectorSize);
		}

		ifs.read((char*)&vertices[i]->weightMethod, sizeof(uint8_t));

		switch (vertices[i]->weightMethod) {
		case 0: // BEF1
			vertices[i]->boneInfo.BDEF.boneIndexes[0] = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
			vertices[i]->boneInfo.BDEF.weights[0] = 1.0f;
			break;
		case 1: // BDEF2
			for (k = 0; k < 2; k++)
				vertices[i]->boneInfo.BDEF.boneIndexes[k] = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
			ifs.read((char*)&vertices[i]->boneInfo.BDEF.weights[0], sizeof(float));
			vertices[i]->boneInfo.BDEF.weights[1] = 1.0f - vertices[i]->boneInfo.BDEF.weights[0];
			break;
		case 2: // BDEF4
		case 4: // QDEF
			for (k = 0; k < 4; k++)
				vertices[i]->boneInfo.BDEF.boneIndexes[k] = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
#ifdef EXTENDED_READ
			for (k = 0; k < 4; k++)
				ifs.read((char*)&vertices[i]->boneInfo.BDEF.weights[k], sizeof(float));
#else
			ifs.read((char*)&vertices[i]->boneInfo.BDEF.weights[0], sizeof(float) * 4);
#endif
			break;
		case 3: // SDEF
			for (k = 0; k < 2; k++)
				vertices[i]->boneInfo.BDEF.boneIndexes[k] = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
			ifs.read((char*)&vertices[i]->boneInfo.SDEF.weightBias, sizeof (float));
			ifs.read((char*)&vertices[i]->boneInfo.SDEF.C, sizeof (float) * 3);
			ifs.read((char*)&vertices[i]->boneInfo.SDEF.R0, sizeof (float) * 3);
			ifs.read((char*)&vertices[i]->boneInfo.SDEF.R1, sizeof (float) * 3);
			break;
		}
		ifs.read((char*)&vertices[i]->edgeWeight, sizeof (float));
		vertices[i]->offset.x = vertices[i]->offset.y = vertices[i]->offset.z = 0.0f;
	}
	delete[] uvExData;

	ifs.read((char*)&count, sizeof (int));
	verticesIndex.resize(count);

	for (int i = 0; i < count; i++) {
		verticesIndex[i] = readAsU32(sizeInfo->cbVertexIndexSize, ifs);
	}

	this->lastpos = ifs.tellg();
	ifs.read((char*)&count, sizeof (int));
	textures.reserve(count);

	for (int i = 0; i < count; i++)
		textures.push_back(getString(ifs));

	ifs.read((char*)&count, sizeof (int));
	materials.resize(count);

	for (int i = 0; i < count; i++) {
		materials[i] = new Material();
		materials[i]->nameJP = getString(ifs);
		materials[i]->nameEN = getString(ifs);
#ifdef EXTENDED_READ
		ifs.read((char*)&materials[i]->diffuse, sizeof (Color4));
		ifs.read((char*)&materials[i]->specular, sizeof (Color));
		ifs.read((char*)&materials[i]->specularCoefficient, sizeof (float));
		ifs.read((char*)&materials[i]->ambient, sizeof (Color));
#else
		ifs.read((char*)&materials[i]->diffuse, sizeof (float) * 11); // diffuse[4], specular[3], specularCoefficient, ambient[3]
#endif
		ifs.read((char*)&materials[i]->flags, sizeof (uint8_t));
#ifdef EXTENDED_READ
		ifs.read((char*)&materials[i]->edgeColor, sizeof (Color4));
		ifs.read((char*)&materials[i]->edgeSize, sizeof (float));
#else
		ifs.read((char*)&materials[i]->edgeColor, sizeof (float) * 5); // edgeColor[4], edgeSize
#endif
		materials[i]->baseTexture = readAsU32(sizeInfo->cbTextureIndexSize, ifs);
		materials[i]->sphereTexture = readAsU32(sizeInfo->cbTextureIndexSize, ifs);
#ifdef EXTENDED_READ
		ifs.read((char*)&materials[i]->sphereMode, sizeof (uint8_t));
		ifs.read((char*)&materials[i]->toonFlag, sizeof (uint8_t));
#else
		ifs.read((char*)&materials[i]->sphereMode, sizeof (uint8_t) * 2); // sphereMode, toonFlag
#endif
		if (materials[i]->toonFlag == 0)
			materials[i]->toonTexture.custom = readAsU32(sizeInfo->cbTextureIndexSize, ifs);
		else
			ifs.read((char*)&materials[i]->toonTexture.default, sizeof (uint8_t));

		materials[i]->freeField = getString(ifs);
		ifs.read((char*)&materials[i]->indexCount, sizeof (int));
	}

	ifs.read((char*)&count, sizeof (int));
	bones.resize(count);
	int subCount;

	for (int i = 0; i < count; i++) {
		bones[i] = new Bone();
		bones[i]->nameJP = getString(ifs);
		bones[i]->nameEN = getString(ifs);
		ifs.read((char*)&bones[i]->position, sizeof (Position));
		bones[i]->parent = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
		ifs.read((char*)&bones[i]->deformation, sizeof (int32_t));
		ifs.read((char*)&bones[i]->flags, sizeof (uint16_t));
		if (bones[i]->flags & BoneFlags::Attached)
			bones[i]->size.attachTo = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
		else 
			ifs.read((char*)&bones[i]->size.length, sizeof (Position));
		if (bones[i]->flags & BoneFlags::InheritRotation) {
			bones[i]->rotationInherit.index = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
			ifs.read((char*)&bones[i]->rotationInherit.rate, sizeof (float));
		}
		if (bones[i]->flags & BoneFlags::TranslateAxis) {
			ifs.read((char*)&bones[i]->axisTranslation, sizeof (Position));
		}
		if (bones[i]->flags & BoneFlags::LocalAxis) {
			ifs.read((char*)&bones[i]->localAxes, sizeof (Position) * 2);
		}
		if (bones[i]->flags & BoneFlags::Flagx2000) {
			ifs.read((char*)&bones[i]->externalDeformationKey, sizeof (int));
		}
		if (bones[i]->flags & BoneFlags::IK) {
			bones[i]->ik.target = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
#ifdef EXTENDED_READ
			ifs.read((char*)&bones[i]->ik.count, sizeof (int));
			ifs.read((char*)&bones[i]->ik.angleLimit, sizeof (float));
#else
			ifs.read((char*)&bones[i]->ik.count, sizeof (int) * 2);
#endif
			ifs.read((char*)&subCount, sizeof (int));
			bones[i]->ik.links.resize(subCount);
			for (int k = 0; k < subCount; k++) {
				IKnode *node = &bones[i]->ik.links[k];
				node->bone = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
				ifs.read((char*)&node->limitAngle, sizeof (bool));
				if (node->limitAngle) {
#ifdef EXTENDED_READ
					ifs.read((char*)&node->limits.lower, sizeof (Position));
					ifs.read((char*)&node->limits.upper, sizeof (Position));
#else
					ifs.read((char*)&node->limits, sizeof (Position) * 2);
#endif
				}
			}
		}
	}

	ifs.read((char*)&count, sizeof (int));
	morphs.resize(count);

	for (int i = 0; i < count; i++) {
		morphs[i] = new Morph();
		morphs[i]->nameJP = getString(ifs);
		morphs[i]->nameEN = getString(ifs);
		ifs.read((char*)&morphs[i]->operation, sizeof (uint8_t));
		ifs.read((char*)&morphs[i]->type, sizeof (uint8_t));
		ifs.read((char*)&subCount, sizeof (int));
		morphs[i]->data.resize(subCount);
		for (int k = 0; k < subCount; k++) {
			switch (morphs[i]->type) {
			case 0: // Group
				morphs[i]->data[k].group.index = readAsU32(sizeInfo->cbMorphIndexSize, ifs);
				ifs.read((char*)&morphs[i]->data[k].group.rate, sizeof (float));
				break;
			case 1: // Vertex
				morphs[i]->data[k].vertex.index = readAsU32(sizeInfo->cbVertexIndexSize, ifs);
				ifs.read((char*)&morphs[i]->data[k].vertex.offset, sizeof (Position));
				break;
			case 2: // Bone
				morphs[i]->data[k].bone.index = readAsU32(sizeInfo->cbVertexIndexSize, ifs);
#ifdef EXTENDED_READ
				ifs.read((char*)&morphs[i]->data[k].bone.movement, sizeof (Position));
				ifs.read((char*)&morphs[i]->data[k].bone.rotation, sizeof (vec4f));
#else
				ifs.read((char*)&morphs[i]->data[k].bone.movement, sizeof (Position) + sizeof (vec4f));
#endif
				break;
			case 3: // UV
			case 4: // UV1
			case 5: // UV2
			case 6: // UV3
			case 7: // UV4
				morphs[i]->data[k].uv.index = readAsU32(sizeInfo->cbVertexIndexSize, ifs);
				ifs.read((char*)&morphs[i]->data[k].uv.offset, sizeof (vec4f));
				break;
			case 8: // Material
				morphs[i]->data[k].material.index = readAsU32(sizeInfo->cbMaterialIndexSize, ifs);
//#ifdef EXTENDED_READ
				ifs.read((char*)&morphs[i]->data[k].material.method, sizeof (uint8_t));
				ifs.read((char*)&morphs[i]->data[k].material.diffuse, sizeof (Color4));
				ifs.read((char*)&morphs[i]->data[k].material.specular, sizeof (Color));
				ifs.read((char*)&morphs[i]->data[k].material.specularCoefficient, sizeof (float));
				ifs.read((char*)&morphs[i]->data[k].material.ambient, sizeof (Color));
				ifs.read((char*)&morphs[i]->data[k].material.edgeColor, sizeof (Color4));
				ifs.read((char*)&morphs[i]->data[k].material.edgeSize, sizeof (float));
				ifs.read((char*)&morphs[i]->data[k].material.baseCoefficient, sizeof (Color4));
				ifs.read((char*)&morphs[i]->data[k].material.sphereCoefficient, sizeof (Color4));
				ifs.read((char*)&morphs[i]->data[k].material.toonCoefficient, sizeof (Color4));
/*#else
				ifs.read((char*)&morphs[i]->data[k].material.method, sizeof (Color4) * 5 + sizeof (Color) * 2 + sizeof (float) * 2 + sizeof (uint8_t));
#endif*/
			}
		}
	}

	ifs.read((char*)&count, sizeof (int));
	frames.resize(count);

	for (int i = 0; i < count; i++) {
		frames[i] = new Frame();
		frames[i]->nameJP = getString(ifs);
		frames[i]->nameEN = getString(ifs);
		ifs.read((char*)&frames[i]->type, sizeof (uint8_t));
		ifs.read((char*)&subCount, sizeof (int));
		frames[i]->morphs.resize(subCount);
		for (int k = 0; k < subCount; k++) {
			ifs.read((char*)&frames[i]->morphs[k].target, sizeof (uint8_t));
			switch (frames[i]->morphs[k].target) {
			case 0:
				frames[i]->morphs[k].id = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
				break;
			case 1:
				frames[i]->morphs[k].id = readAsU32(sizeInfo->cbMorphIndexSize, ifs);
				break;
			}
		}
	}

	ifs.read((char*)&count, sizeof (int));
	bodies.resize(count);

	for (int i = 0; i < count; i++) {
		bodies[i] = new RigidBody();
		bodies[i]->nameJP = getString(ifs);
		bodies[i]->nameEN = getString(ifs);
		bodies[i]->targetBone = readAsU32(sizeInfo->cbBoneIndexSize, ifs);
#ifdef EXTENDED_READ
		ifs.read((char*)&bodies[i]->group, sizeof (uint8_t));
		ifs.read((char*)&bodies[i]->nonCollisionFlag, sizeof (uint16_t));
		ifs.read((char*)&bodies[i]->shape, sizeof (uint8_t));
		ifs.read((char*)&bodies[i]->size, sizeof (vec3f));
		ifs.read((char*)&bodies[i]->position, sizeof (vec3f));
		ifs.read((char*)&bodies[i]->rotation, sizeof (vec3f));
		ifs.read((char*)&bodies[i]->mass, sizeof (float));
		ifs.read((char*)&bodies[i]->inertia, sizeof (float));
		ifs.read((char*)&bodies[i]->rotationDampening, sizeof (float));
		ifs.read((char*)&bodies[i]->repulsion, sizeof (float));
		ifs.read((char*)&bodies[i]->friction, sizeof (float));
		ifs.read((char*)&bodies[i]->mode, sizeof (uint8_t));
#else
		ifs.read((char*)&bodies[i]->group, sizeof (vec3f) * 3 + sizeof (float) * 5 + sizeof (uint8_t) * 3 + sizeof (uint16_t));
#endif
	}

	ifs.read((char*)&count, sizeof (int));
	joints.resize(count);

	for (int i = 0; i < count; i++) {
		joints[i] = new Joint();
		joints[i]->nameJP = getString(ifs);
		joints[i]->nameEN = getString(ifs);
		ifs.read((char*)&joints[i]->type, sizeof (uint8_t));
		switch (joints[i]->type) {
		case 0:
			joints[i]->spring.bodyA = readAsU32(sizeInfo->cbRigidBodyIndexSize, ifs);
			joints[i]->spring.bodyB = readAsU32(sizeInfo->cbRigidBodyIndexSize, ifs);
#ifdef EXTENDED_READ
			ifs.read((char*)&joints[i]->spring.position, sizeof (vec3f));
			ifs.read((char*)&joints[i]->spring.rotation, sizeof (vec3f));
			ifs.read((char*)&joints[i]->spring.lowerMovementRestrictions, sizeof (vec3f));
			ifs.read((char*)&joints[i]->spring.upperMovementRestrictions, sizeof (vec3f));
			ifs.read((char*)&joints[i]->spring.lowerRotationRestrictions, sizeof (vec3f));
			ifs.read((char*)&joints[i]->spring.upperRotationRestrictions, sizeof (vec3f));
			ifs.read((char*)&joints[i]->spring.movementSpringConstant, sizeof (vec3f));
			ifs.read((char*)&joints[i]->spring.rotationSpringConstant, sizeof (vec3f));
#else
			ifs.read((char*)&joints[i]->spring.position, sizeof (vec3f) * 8);
#endif
			break;
		}
	}

	lastpos = ifs.tellg();
	ifs.close();

	return true;
}

bool PMX::Model::RenderBuffers(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<CameraClass> camera, std::shared_ptr<ViewFrustum> frustum)
{
	unsigned int stride;
	unsigned int offset;
	DirectX::XMFLOAT3 cameraPosition;
	ID3D11DeviceContext *context = d3d->GetDeviceContext();
	Shaders::Light::MaterialBufferType matBuf;;

	DirectX::XMStoreFloat3(&cameraPosition, camera->GetPosition());

	ID3D11ShaderResourceView *textures[3];

	int renderedMaterials = 0;
    
	for (int i = 0; i < rendermaterials.size(); i++) {
		// Check if we need to render this material
		if (!frustum->IsBoxInside(rendermaterials[i].center, rendermaterials[i].radius))
			continue; // Material is not inside the view frustum, so just skip

		renderedMaterials ++;
		
		// Set vertex buffer stride and offset.
		stride = sizeof(VertexType); 
		offset = 0;

		if (materials[i]->flags & MaterialFlags::DoubleSide == MaterialFlags::DoubleSide ||
			materials[i]->diffuse.alpha < 1.0f) {
			// Enable no-culling rasterizer
			context->RSSetState(d3d->GetRasterState(1));
		}

		// Set the vertex buffer to active in the input assembler so it can be rendered.
		context->IASetVertexBuffers(0, 1, &rendermaterials[i].vertexBuffer, &stride, &offset);

		// Set the index buffer to active in the input assembler so it can be rendered.
		context->IASetIndexBuffer(rendermaterials[i].indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		textures[0] = rendermaterials[i].baseTexture ? rendermaterials[i].baseTexture->GetTexture() : NULL;
		textures[1] = rendermaterials[i].sphereTexture ? rendermaterials[i].sphereTexture->GetTexture() : NULL;
		textures[2] = rendermaterials[i].toonTexture ? rendermaterials[i].toonTexture->GetTexture() : NULL;

		matBuf.flags = 0;
		matBuf.flags |= materials[i]->sphereMode != MaterialSphereMode::Disabled ? 0x01 : 0;
		matBuf.flags |= materials[i]->sphereMode == MaterialSphereMode::Add ? 0x02 : 0;
		matBuf.flags |= textures[2] != NULL ? 0x04 : 0;

		if ((matBuf.flags & 0x04) == 0) {
			matBuf.diffuseColor = rendermaterials[i].getDiffuse(materials[i]);
			matBuf.ambientColor  = rendermaterials[i].getAmbient(materials[i]);
		}
		else {
			matBuf.diffuseColor = matBuf.ambientColor = rendermaterials[i].getAverage(materials[i]);
		}
		matBuf.specularColor = rendermaterials[i].getSpecular(materials[i]);

		if (!lightShader->Render(context, 
								 this->rendermaterials[i].indexCount,
								 world,
								 view,
								 projection,
								 textures,
								 3,
								 light->GetDirection(),
								 light->GetDiffuseColor(),
								 light->GetAmbientColor(),
								 cameraPosition,
								 light->GetSpecularColor(),
								 rendermaterials[i].getSpecularCoefficient(materials[i]),
								 matBuf))
			return false;

		if (materials[i]->flags & MaterialFlags::DoubleSide == MaterialFlags::DoubleSide ||
			materials[i]->diffuse.alpha < 1.0f) {
			// Return to default rasterizer
			context->RSSetState(d3d->GetRasterState(0));
		}
	}

	return true;
}

bool PMX::Model::LoadTexture(ID3D11Device *device)
{
	if (sharedToonTextures.size() != defaultToonTexCount) {
		sharedToonTextures.resize(defaultToonTexCount);

		for (int i = 0; i < defaultToonTexCount; i++) {
			sharedToonTextures[i].reset(new Texture);

			if (!sharedToonTextures[i]->Initialize(device, defaultToonTexs[i]))
				return false;
		}
	}

	renderTextures.resize(textures.size());
	for (int i = 0; i < renderTextures.size(); i++) {
		renderTextures[i].reset(new Texture);

		if (!renderTextures[i]->Initialize(device, basePath + textures[i])) {
			renderTextures[i].reset();
		}
	}

	for (int i = 0; i < rendermaterials.size(); i++) {
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
	for (int i = 0; i < renderTextures.size(); i++) {
		if (renderTextures[i] != nullptr) {
			renderTextures[i]->Shutdown();
			renderTextures[i].reset();
		}
	}

	renderTextures.resize(0);
}

PMX::Position PMX::Model::GetBonePosition(const std::wstring &nameJP)
{
	for (auto i : bones) {
		if (i->nameJP.compare(nameJP) == 0) {
			return i->position;
		}
	}

	// Return root bone position if not found
	return bones[0]->position;
}

PMX::Position PMX::Model::GetBoneEndPosition(const std::wstring &nameJP)
{
	PMX::Position pos = bones[0]->position;

	for (auto i : bones) {
		if (i->nameJP.compare(nameJP) == 0) {
			if (i->flags & BoneFlags::Attached) {
				return bones[i->size.attachTo]->position;
			}

			pos.x = i->position.x + i->size.length.x;
			pos.y = i->position.y + i->size.length.y;
			pos.z = i->position.z + i->size.length.z;
		}
	}

	// Return root bone position if not found
	return pos;
}

void PMX::Model::ApplyMorph(const std::wstring &nameJP, float weight)
{
	for (auto i : morphs) {
		if (i->nameJP.compare(nameJP) == 0) {
			switch (i->type) {
			case MorphType::Group: // Group Morph
				break;
			case MorphType::Vertex:
				applyVertexMorph(i, weight);
				break;
			case MorphType::Bone:
				break;
			case MorphType::Material:
				applyMaterialMorph(i, weight);
				break;
			}
		}
	}
}

void PMX::Model::applyVertexMorph(Morph *morph, float weight)
{
	std::map<uint32_t, RenderMaterial*> materialsToUpdate;

	for (auto i : morph->data) {
		Vertex *v = vertices[i.vertex.index];

		v->offset.x = i.vertex.offset.x * weight;
		v->offset.y = i.vertex.offset.y * weight;
		v->offset.z = i.vertex.offset.z * weight;

		materialsToUpdate[v->material->materialIndex] = v->material;
	}

	// update the vertex buffers
	for (auto i : materialsToUpdate) {
		updateMaterialVertexBuffer(i.second);
	}
}

void PMX::Model::applyMaterialMorph(Morph *morph, float weight)
{
	for (auto i : morph->data) {
		if (i.material.index == -1) { // If -1, apply morph to all materials
			for (auto m : rendermaterials)
				applyMaterialMorph(&i, &m, weight);
		}
		else {
			applyMaterialMorph(&i, &rendermaterials[i.material.index], weight);
		}
	}
}

void PMX::Model::applyMaterialMorph(MorphType *morph, PMX::RenderMaterial *material, float weight)
{
	if (morph->material.method == 0) // multiplicative
		material->multiplicative = morph->material;
	else if (morph->material.method == 1) // additive
		material->additive = morph->material;
}

void PMX::Model::updateMaterialVertexBuffer(PMX::RenderMaterial *material)
{
	ID3D11Device *device;
	ID3D11DeviceContext *context;
	VertexType *vertices;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	Vertex *vertex;
	HRESULT result;

	// get the d3d11 device and context from the vertex buffer
	material->vertexBuffer->GetDevice(&device);
	device->GetImmediateContext(&context);

	// get access to the buffer
	result = context->Map(material->vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
		return;

	vertices = (VertexType*)mappedResource.pData;
	// Update vertex data
	for (int i = 0; i < material->indexCount; i++) {
		vertex = this->vertices[this->verticesIndex[i + material->startIndex]];
		vertices[i].position.x = vertex->position.x + vertex->offset.x;
		vertices[i].position.y = vertex->position.y + vertex->offset.y;
		vertices[i].position.z = vertex->position.z + vertex->offset.z;
		vertices[i].normal = DirectX::XMFLOAT3(vertex->normal.x, vertex->normal.y, vertex->normal.z);
		vertices[i].texture = DirectX::XMFLOAT2(vertex->uv[0], vertex->uv[1]);
	}

	// release the access to the buffer
	context->Unmap(material->vertexBuffer, 0);
}

void PMX::Model::generateAdditiveMaterialMorph(MaterialMorph &morph)
{
	morph.ambient.red = morph.ambient.green = morph.ambient.blue = 0.0f;
	morph.baseCoefficient.red = morph.baseCoefficient.green = morph.baseCoefficient.blue = morph.baseCoefficient.alpha = 0.0f;
	morph.diffuse.red = morph.diffuse.green = morph.diffuse.blue = morph.diffuse.alpha = 0.0f;
	morph.edgeColor.red = morph.edgeColor.green = morph.edgeColor.blue = morph.edgeColor.alpha = 0.0f;
	morph.edgeSize = 0.0f;
	morph.index = 0;
	morph.method = 1;
	morph.specular.red = morph.specular.green = morph.specular.blue = 0.0f;
	morph.specularCoefficient = 0.0f;
	morph.sphereCoefficient.red = morph.sphereCoefficient.green = morph.sphereCoefficient.blue = morph.sphereCoefficient.alpha = 0.0f;
	morph.toonCoefficient.red = morph.toonCoefficient.green = morph.toonCoefficient.blue = morph.toonCoefficient.alpha = 0.0f;
}

void PMX::Model::generateMultiplicativeMaterialMorph(MaterialMorph &morph)
{
	morph.ambient.red = morph.ambient.green = morph.ambient.blue = 1.0f;
	morph.baseCoefficient.red = morph.baseCoefficient.green = morph.baseCoefficient.blue = morph.baseCoefficient.alpha = 1.0f;
	morph.diffuse.red = morph.diffuse.green = morph.diffuse.blue = morph.diffuse.alpha = 1.0f;
	morph.edgeColor.red = morph.edgeColor.green = morph.edgeColor.blue = morph.edgeColor.alpha = 1.0f;
	morph.edgeSize = 1.0f;
	morph.index = 0;
	morph.method = 0;
	morph.specular.red = morph.specular.green = morph.specular.blue = 1.0f;
	morph.specularCoefficient = 1.0f;
	morph.sphereCoefficient.red = morph.sphereCoefficient.green = morph.sphereCoefficient.blue = morph.sphereCoefficient.alpha = 1.0f;
	morph.toonCoefficient.red = morph.toonCoefficient.green = morph.toonCoefficient.blue = morph.toonCoefficient.alpha = 1.0f;
}

DirectX::XMFLOAT4 PMX::RenderMaterial::getDiffuse(PMX::Material *m)
{
	return DirectX::XMFLOAT4(m->diffuse.red * multiplicative.diffuse.red + additive.diffuse.red,
		m->diffuse.green * multiplicative.diffuse.green + additive.diffuse.green,
		m->diffuse.blue * multiplicative.diffuse.blue + additive.diffuse.blue,
		m->diffuse.alpha * multiplicative.diffuse.alpha + additive.diffuse.alpha);
}

DirectX::XMFLOAT4 PMX::RenderMaterial::getAmbient(PMX::Material *m)
{
	return DirectX::XMFLOAT4(m->ambient.red * multiplicative.ambient.red + additive.ambient.red,
		m->ambient.green * multiplicative.ambient.green + additive.ambient.green,
		m->ambient.blue * multiplicative.ambient.blue + additive.ambient.blue,
		1.0f);
}

DirectX::XMFLOAT4 PMX::RenderMaterial::getAverage(PMX::Material *m)
{
	DirectX::XMFLOAT4 d = getDiffuse(m), a = getAmbient(m);

	return DirectX::XMFLOAT4((d.x + a.x)/2.0f, (d.y + a.y) / 2.0f, (d.z + a.z) / 2.0f, (d.w + a.w) / 2.0f);
}

DirectX::XMFLOAT4 PMX::RenderMaterial::getSpecular(PMX::Material *m)
{
	return DirectX::XMFLOAT4(m->specular.red * multiplicative.specular.red + additive.specular.red,
		m->specular.green * multiplicative.specular.green + additive.specular.green,
		m->specular.blue * multiplicative.specular.blue + additive.specular.blue,
		1.0f);
}

float PMX::RenderMaterial::getSpecularCoefficient(PMX::Material *m)
{
	return m->specularCoefficient * multiplicative.specularCoefficient + additive.specularCoefficient;
}

