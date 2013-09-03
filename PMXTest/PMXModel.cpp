#include "stdafx.h"
#include "PMXModel.h"

#include <fstream>
#include <cstring>
#include <codecvt>

using namespace std;

//#define EXTENDED_READ

PMX::Model::Model(void)
{
	header = new Header();
	sizeInfo = new SizeInfo();
}


PMX::Model::~Model(void)
{
	delete header;
	delete sizeInfo;

	for (std::vector<PMX::Vertex*>::size_type i = 0; i < vertices.size(); i++) {
		delete vertices[i];
		vertices[i] = nullptr;
	}

	for (std::vector<PMX::Material*>::size_type i = 0; i < materials.size(); i++) {
		delete materials[i];
		materials[i] = nullptr;
	}

	for (std::vector<PMX::Bone*>::size_type i = 0; i < bones.size(); i++) {
		delete bones[i];
		bones[i] = nullptr;
	}

	for (std::vector<PMX::Morph*>::size_type i = 0; i < morphs.size(); i++) {
		delete morphs[i];
		morphs[i] = nullptr;
	}

	for (std::vector<PMX::Frame*>::size_type i = 0; i < frames.size(); i++) {
		delete frames[i];
		frames[i] = nullptr;
	}
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
		return u8val;
	case 2:
		is.read((char*)&u16val, sizeof (uint16_t));
		return u16val;
	case 4:
		is.read((char*)&u32val, sizeof (uint32_t));
		return u32val;
	}

	return 0;
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
			for (k = 0; k < 4; k++)
				ifs.read((char*)&vertices[i]->boneInfo.BDEF.weights[k], sizeof(float));
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
	}
	delete[] uvExData;

	ifs.read((char*)&count, sizeof (int));
	verticesIndex.reserve(count);

	for (int i = 0; i < count; i++)
		verticesIndex.push_back(readAsU32(sizeInfo->cbVertexIndexSize, ifs));

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
		ifs.read((char*)&materials[i]->materalType, sizeof (int));
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
#ifdef EXTENDED_READ
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
#else
				ifs.read((char*)&morphs[i]->data[k].material.method, sizeof (Color4) * 5 + sizeof (Color) * 2 + sizeof (float) * 2 + sizeof (uint8_t));
#endif
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
