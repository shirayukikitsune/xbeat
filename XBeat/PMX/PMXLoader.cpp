//===-- PMX/PMXLoader.cpp - Defines the PMX loading class ------------*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares the PMX::Loader class
///
//===---------------------------------------------------------------------------===//

#include "PMXBone.h"
#include "PMXModel.h"

#include <codecvt>

using namespace std;
using namespace PMX;

bool Loader::loadFromFile(Model* model, const std::wstring &filename)
{
	std::ifstream ifile;
	ifile.open(filename, std::ios::binary);
	if (!ifile.good())
		return false;

	bool ret = loadFromStream(model, ifile);
	ifile.close();
	return ret;
}

bool Loader::loadFromStream(Model* model, std::istream &in)
{
	std::istream::pos_type pos = in.tellg();
	in.seekg(0, in.end);
	std::istream::pos_type len = in.tellg();
	len -= pos;
	in.seekg(pos, in.beg);

	char *data = new char[len];
	const char *cursor = data;

	in.read(data, len);
	bool ret = loadFromMemory(model, cursor);
	in.seekg(pos + (std::istream::pos_type)(cursor - data), in.beg);

	delete[] data;

	return ret;
}

bool Loader::loadFromMemory(Model* model, const char *&data)
{
	Header = loadHeader(data);
	// Check if we have a valid header
	if (Header == nullptr)
		return false;

	SizeInfo = loadSizeInfo(data);

	loadDescription(model->description, data);

	loadVertexData(model, data);

	loadIndexData(model, data);

	loadTextures(model, data);

	loadMaterials(model, data);

	loadBones(model, data);

	loadMorphs(model, data);

	loadFrames(model, data);

	loadRigidBodies(model, data);

	loadJoints(model, data);

	if (Header->Version >= 2.1f)
		loadSoftBodies(model, data);

	return true;
}

ModelDescription Loader::getDescription(const std::wstring &filename)
{
	std::ifstream ifile;
	ifile.open(filename, std::ios::binary);
	if (!ifile.good())
		throw Exception("Unable to open the requested filename");

	ifile.seekg(0, ifile.end);
	std::istream::pos_type len = ifile.tellg();
	ifile.seekg(0, ifile.beg);

	char *data = new char[len];
	const char *cursor = data;

	ifile.read(data, len);

	auto header = loadHeader(cursor);
	if (header == nullptr) throw Exception("Unrecognized file format");
	SizeInfo = loadSizeInfo(cursor);
	ModelDescription output;
	loadDescription(output, cursor);

	delete[] data;
	ifile.close();

	return output;
}

template <class T>
void readInfo(T& value, const char *&data)
{
	memcpy(&value, data, sizeof (T));
	data += sizeof (T);
}
template <class T>
T readInfo(const char *&data)
{
	T value;
	memcpy(&value, data, sizeof (T));
	data += sizeof (T);

	return value;
}
template <class T>
void readVector(T* vec, size_t size, const char *&data)
{
	for (size_t s = 0; s < size; s++) {
		readInfo<T>(vec[s], data);
	}
}

Loader::FileHeader* Loader::loadHeader(const char *&data)
{
	FileHeader *header = (FileHeader*)data;
	data += sizeof FileHeader;

	// Check file signature
	if (strncmp("Pmx ", header->Magic, 4) != 0 && strncmp("PMX ", header->Magic, 4) != 0)
		return nullptr;

	// Check supported versions
	if (header->Version != 2.0f && header->Version != 2.1f)
		return nullptr;

	return header;
}

Loader::FileSizeInfo* Loader::loadSizeInfo(const char *&data)
{
	FileSizeInfo *sizeInfo = (FileSizeInfo*)data;
	data += sizeof FileSizeInfo;

	return sizeInfo;
}

void Loader::loadDescription(ModelDescription &desc, const char *&data)
{
	readName(desc.name, data);
	readName(desc.comment, data);
}

void Loader::loadVertexData(Model *model, const char*& data) {
	model->vertices.resize(readInfo<int>(data));

	int i;
	uint32_t id = 0;

	for (auto &vertex : model->vertices)
	{
		vertex = new Vertex;
		readVector<float>(&vertex->position.x, 3, data);
		readVector<float>(&vertex->normal.x, 3, data);
		readVector<float>(vertex->uv, 2, data);
		readVector<float>(&vertex->uvEx[0].x, SizeInfo->UVVectorSize, data);
		vertex->weightMethod = readInfo<VertexWeightMethod>(data);
		std::memset(&vertex->boneInfo, 0, sizeof(vertex->boneInfo));
		switch (vertex->weightMethod)
		{
		case VertexWeightMethod::BDEF1:
			vertex->boneInfo.BDEF.boneIndexes[0] = readAsU32(SizeInfo->BoneIndexSize, data);
			vertex->boneInfo.BDEF.weights[0] = 1.0f;
			break;
		case VertexWeightMethod::BDEF2:
			for (i = 0; i < 2; i++)
				vertex->boneInfo.BDEF.boneIndexes[i] = readAsU32(SizeInfo->BoneIndexSize, data);

			vertex->boneInfo.BDEF.weights[0] = readInfo<float>(data);
			vertex->boneInfo.BDEF.weights[1] = 1.0f - vertex->boneInfo.BDEF.weights[0];
			break;
		case VertexWeightMethod::QDEF:
			if (Header->Version < 2.1f)
				throw Exception("QDEF not supported on PMX version lower than 2.1");
		case VertexWeightMethod::BDEF4:
			for (i = 0; i < 4; i++)
				vertex->boneInfo.BDEF.boneIndexes[i] = readAsU32(SizeInfo->BoneIndexSize, data);
			readVector(vertex->boneInfo.BDEF.weights, 4, data);
			break;
		case VertexWeightMethod::SDEF:
			vertex->boneInfo.SDEF.boneIndexes[0] = readAsU32(SizeInfo->BoneIndexSize, data);
			vertex->boneInfo.SDEF.boneIndexes[1] = readAsU32(SizeInfo->BoneIndexSize, data);
			vertex->boneInfo.SDEF.weightBias = readInfo<float>(data);
			readVector<float>(vertex->boneInfo.SDEF.C, 3, data);
			readVector<float>(vertex->boneInfo.SDEF.R0, 3, data);
			readVector<float>(vertex->boneInfo.SDEF.R1, 3, data);
			break;
		default:
			throw Exception("Invalid value for vertex weight method");
		}
		vertex->edgeWeight = readInfo<float>(data);
		vertex->index = id++;

		// Set some defaults
		for (int i = 0; i < 4; i++) {
			vertex->bones[i] = nullptr;
		}

		vertex->MorphOffset = DirectX::XMVectorZero();
	}
}

void Loader::loadIndexData(Model *model, const char *&data)
{
	model->verticesIndex.resize(readInfo<int>(data));

	for (auto &index : model->verticesIndex)
	{
		index = readAsU32(SizeInfo->VertexIndexSize, data);
	}
}

void Loader::loadTextures(Model *model, const char *&data)
{
	model->textures.resize(readInfo<int>(data));

	for (auto &texture : model->textures)
	{
		texture = getString(data);
	}
}

void Loader::loadMaterials(Model *model, const char *&data)
{
	model->materials.resize(readInfo<int>(data));

	for (auto &material : model->materials)
	{
		material = new Material;
		readName(material->name, data);

		readInfo<Color4>(material->diffuse, data);
		readInfo<Color>(material->specular, data);
		material->specularCoefficient = readInfo<float>(data);
		readInfo<Color>(material->ambient, data);

		material->flags = readInfo<uint8_t>(data);
		
		readInfo<Color4>(material->edgeColor, data);
		material->edgeSize = readInfo<float>(data);

		material->baseTexture = readAsU32(SizeInfo->MaterialIndexSize, data);
		material->sphereTexture = readAsU32(SizeInfo->MaterialIndexSize, data);
		material->sphereMode = readInfo<MaterialSphereMode>(data);
		material->toonFlag = readInfo<MaterialToonMode>(data);

		switch (material->toonFlag) {
		case MaterialToonMode::DefaultTexture:
			material->toonTexture.default = readInfo<uint8_t>(data);
			break;
		case MaterialToonMode::CustomTexture:
			material->toonTexture.custom = readAsU32(SizeInfo->TextureIndexSize, data);
			break;
		}

		material->freeField = getString(data);

		material->indexCount = readInfo<int>(data);
	}
}

void Loader::loadBones(Model *model, const char *&data)
{
	Bones.resize(readInfo<int>(data));

	uint32_t id = 0;

	for (auto &Bone : Bones)
	{
		readName(Bone.Name, data);

		readVector<float>(&Bone.InitialPosition.x, 3, data);
		Bone.Parent = readAsU32(SizeInfo->BoneIndexSize, data);
		Bone.DeformationOrder = readInfo<int>(data);

		Bone.Flags = readInfo<uint16_t>(data);

		if (Bone.Flags & (uint16_t)BoneFlags::Attached)
			Bone.Size.AttachTo = readAsU32(SizeInfo->BoneIndexSize, data);
		else readVector<float>(Bone.Size.Length, 3, data);

		if (Bone.Flags & ((uint16_t)BoneFlags::RotationAttached | (uint16_t)BoneFlags::TranslationAttached)) {
			Bone.Inherit.From = readAsU32(SizeInfo->BoneIndexSize, data);
			Bone.Inherit.Rate = readInfo<float>(data);
		}
		else {
			Bone.Inherit.From = 0xFFFFFFFFU;
			Bone.Inherit.Rate = 1.0f;
		}

		if (Bone.Flags & (uint16_t)BoneFlags::FixedAxis) {
			readVector<float>(&Bone.AxisTranslation.x, 3, data);
		}

		if (Bone.Flags & (uint16_t)BoneFlags::LocalAxis) {
			readVector<float>(&Bone.LocalAxes.X.x, 3, data);
			readVector<float>(&Bone.LocalAxes.Z.x, 3, data);
		}

		if (Bone.Flags & (uint16_t)BoneFlags::OuterParentDeformation) {
			Bone.ExternalDeformationKey = readInfo<int>(data);
		}

		if (Bone.Flags & (uint16_t)BoneFlags::IK) {
			Bone.IkData = new IK;
			Bone.IkData->targetIndex = readAsU32(SizeInfo->BoneIndexSize, data);
			Bone.IkData->loopCount = readInfo<int>(data);
			Bone.IkData->angleLimit = readInfo<float>(data);

			Bone.IkData->links.resize(readInfo<int>(data));
			for (auto &Link : Bone.IkData->links) {
				Link.boneIndex = readAsU32(SizeInfo->BoneIndexSize, data);
				Link.limitAngle = readInfo<bool>(data);
				if (Link.limitAngle) {
					readVector<float>(Link.limits.lower, 3, data);
					readVector<float>(Link.limits.upper, 3, data);
				}
			}
		}
		else Bone.IkData = nullptr;
	}
}

void Loader::loadMorphs(Model *model, const char *&data)
{
	model->morphs.resize(readInfo<int>(data));
	
	for (auto &morph : model->morphs)
	{
		morph = new Morph;
		readName(morph->name, data);

		morph->operation = readInfo<uint8_t>(data);
		morph->type = readInfo<uint8_t>(data);
		morph->data.resize(readInfo<int>(data));

		for (auto &mdata : morph->data)
		{
			switch (morph->type)
			{
			case MorphType::Flip:
				if (Header->Version < 2.1f)
					throw Exception("Flip morph not available for PMX version < 2.1");
			case MorphType::Group:
				mdata.group.index = readAsU32(SizeInfo->MorphIndexSize, data);
				mdata.group.rate = readInfo<float>(data);
				break;
			case MorphType::Vertex:
				mdata.vertex.index = readAsU32(SizeInfo->VertexIndexSize, data);
				readVector<float>(mdata.vertex.offset, 3, data);
				break;
			case MorphType::Bone:
				mdata.bone.index = readAsU32(SizeInfo->VertexIndexSize, data);
				readVector<float>(mdata.bone.movement, 3, data);
				readVector<float>(mdata.bone.rotation, 4, data);
				break;
			case MorphType::UV:
			case MorphType::UV1:
			case MorphType::UV2:
			case MorphType::UV3:
			case MorphType::UV4:
				mdata.uv.index = readAsU32(SizeInfo->VertexIndexSize, data);
				readVector<float>(mdata.uv.offset, 4, data);
				break;
			case MorphType::Material:
				mdata.material.index = readAsU32(SizeInfo->MaterialIndexSize, data);
				mdata.material.method = readInfo<MaterialMorphMethod>(data);
				readInfo<Color4>(mdata.material.diffuse, data);
				readInfo<Color>(mdata.material.specular, data);
				mdata.material.specularCoefficient = readInfo<float>(data);
				readInfo<Color>(mdata.material.ambient, data);
				readInfo<Color4>(mdata.material.edgeColor, data);
				mdata.material.edgeSize = readInfo<float>(data);
				readInfo<Color4>(mdata.material.baseCoefficient, data);
				readInfo<Color4>(mdata.material.sphereCoefficient, data);
				readInfo<Color4>(mdata.material.toonCoefficient, data);
				break;
			case MorphType::Impulse:
				if (Header->Version < 2.1f)
					throw Exception("Impulse morph not supported in PMX version < 2.1");

				mdata.impulse.index = readAsU32(SizeInfo->RigidBodyIndexSize, data);
				mdata.impulse.localFlag = readInfo<uint8_t>(data);
				readVector<float>(mdata.impulse.velocity, 3, data);
				readVector<float>(mdata.impulse.rotationTorque, 3, data);
				break;
			default:
				throw Exception("Invalid morph type");
			}
		}
	}
}

void Loader::loadFrames(Model *model, const char *&data)
{
	model->frames.resize(readInfo<int>(data));

	for (auto &frame : model->frames)
	{
		frame = new Frame;
		readName(frame->name, data);
		frame->type = readInfo<uint8_t>(data);
		frame->morphs.resize(readInfo<int>(data));

		for (auto &morph : frame->morphs)
		{
			morph.target = readInfo<FrameMorphTarget>(data);
			switch (morph.target) {
			case FrameMorphTarget::Bone:
				morph.id = readAsU32(SizeInfo->BoneIndexSize, data);
				break;
			case FrameMorphTarget::Morph:
				morph.id = readAsU32(SizeInfo->MorphIndexSize, data);
				break;
			}
		}
	}
}

void Loader::loadRigidBodies(Model *Model, const char *&Data)
{
	RigidBodies.resize(readInfo<int>(Data));

	for (auto &Body : RigidBodies)
	{
		readName(Body.name, Data);

		Body.targetBone = readAsU32(SizeInfo->BoneIndexSize, Data);

		Body.group = readInfo<uint8_t>(Data);
		Body.groupMask = readInfo<uint16_t>(Data);

		Body.shape = readInfo<PMX::RigidBodyShape>(Data);
		readVector<float>(&Body.size.x, 3, Data);

		readVector<float>(&Body.position.x, 3, Data);
		readVector<float>(&Body.rotation.x, 3, Data);

		Body.mass = readInfo<float>(Data);

		Body.linearDamping = readInfo<float>(Data);
		Body.angularDamping = readInfo<float>(Data);
		Body.restitution = readInfo<float>(Data);
		Body.friction = readInfo<float>(Data);

		Body.mode = readInfo<PMX::RigidBodyMode>(Data);
	}
}

void Loader::loadJoints(Model *model, const char *&data)
{
	Joints.resize(readInfo<int>(data));

	for (auto &Joint : Joints)
	{
		readName(Joint.name, data);

		Joint.type = readInfo<JointType>(data);

		if (Header->Version < 2.1f && Joint.type != JointType::Spring6DoF)
			throw Exception("Invalid joint type for PMX version < 2.1");

		Joint.data.bodyA = readAsU32(SizeInfo->RigidBodyIndexSize, data);
		Joint.data.bodyB = readAsU32(SizeInfo->RigidBodyIndexSize, data);

		readVector<float>(&Joint.data.position.x, 3, data);
		readVector<float>(&Joint.data.rotation.x, 3, data);

		readVector<float>(&Joint.data.lowerMovementRestrictions.x, 3, data);
		readVector<float>(&Joint.data.upperMovementRestrictions.x, 3, data);
		readVector<float>(&Joint.data.lowerRotationRestrictions.x, 3, data);
		readVector<float>(&Joint.data.upperRotationRestrictions.x, 3, data);

		readVector<float>(Joint.data.springConstant, 6, data);
	}
}

void Loader::loadSoftBodies(Model *model, const char *&data)
{
	model->softBodies.resize(readInfo<int>(data));

	for (auto &body : model->softBodies)
	{
		body = new SoftBody;
		readName(body->name, data);

		body->shape = readInfo<SoftBody::Shape::Shape_e>(data);
		body->material = readAsU32(SizeInfo->MaterialIndexSize, data);

		body->group = readInfo<uint8_t>(data);
		body->groupFlags = readInfo<uint16_t>(data);

		body->flags = readInfo<SoftBody::Flags::Flags_e>(data);

		body->blinkCreationDistance = readInfo<int>(data);
		body->clusterCount = readInfo<int>(data);

		body->mass = readInfo<float>(data);
		body->collisionMargin = readInfo<float>(data);

		body->model = readInfo<SoftBody::AeroModel::AeroModel_e>(data);

		readInfo<SoftBody::Config>(body->config, data);
		readInfo<SoftBody::Cluster>(body->cluster, data);
		readInfo<SoftBody::Iteration>(body->iteration, data);
		readInfo<SoftBody::Material>(body->materialInfo, data);

		body->anchors.resize(readInfo<int>(data));
		for (auto &anchor : body->anchors)
		{
			anchor.rigidBodyIndex = readAsU32(SizeInfo->RigidBodyIndexSize, data);
			anchor.vertexIndex = readAsU32(SizeInfo->VertexIndexSize, data);
			anchor.nearMode = readInfo<uint8_t>(data);
		}

		int count = readInfo<int>(data);
		body->pins.resize(count);
		readVector<SoftBody::Pin>(body->pins.data(), count, data);
	}
}

template <class T>
T __getString(const char *&data)
{
	// Read length
	uint32_t len = readInfo<uint32_t>(data);
	len /= sizeof (T::value_type);

	// Read the string itself
	T retval((T::pointer)data, len);
	data += len * sizeof(T::value_type);
	return retval;
}

std::wstring Loader::getString(const char *&data) {
	static wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> conversor;

	switch (SizeInfo->Encoding) {
	case 0:
		return __getString<wstring>(data);
	case 1:
		return conversor.from_bytes(__getString<string>(data));
	}

	return L"";
}

void Loader::readName(Name &name, const char *&data)
{
	name.japanese = getString(data);
	name.english = getString(data);
}

uint32_t Loader::readAsU32(uint8_t size, const char *&data)
{
	uint8_t u8val;
	uint16_t u16val;
	uint32_t u32val;

	switch (size) {
	case 1:
		u8val = *(uint8_t*)data;
		data += size;
		if (u8val == 0xFF) return 0xFFFFFFFF;
		return u8val;
	case 2:
		u16val = *(uint16_t*)data;
		data += size;
		if (u16val == 0xFFFF) return 0xFFFFFFFF;
		return u16val;
	case 4:
		u32val = *(uint32_t*)data;
		data += size;
		return u32val;
	}

	return 0;
}
