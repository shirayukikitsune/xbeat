#include "PMXModel.h"
#include "PMXBone.h"

#include <codecvt>

using namespace std;
using namespace PMX;

bool Loader::FromFile(Model* model, const std::wstring &filename)
{
	std::ifstream ifile;
	ifile.open(filename, std::ios::binary);
	if (!ifile.good())
		return false;

	bool ret = FromStream(model, ifile);
	ifile.close();
	return ret;
}

bool Loader::FromStream(Model* model, std::istream &in)
{
	std::istream::pos_type pos = in.tellg();
	in.seekg(0, in.end);
	std::istream::pos_type len = in.tellg();
	len -= pos;
	in.seekg(pos, in.beg);

	char *data = new char[len];
	const char *cursor = data;

	in.read(data, len);
	bool ret = FromMemory(model, cursor);
	in.seekg(pos + (std::istream::pos_type)(cursor - data), in.beg);

	delete[] data;

	return ret;
}

bool Loader::FromMemory(Model* model, const char *&data)
{
	m_header = loadHeader(data);
	// Check if we have a valid header
	if (m_header == nullptr)
		return false;

	m_sizeInfo = loadSizeInfo(data);

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

	if (m_header->fVersion >= 2.1f)
		loadSoftBodies(model, data);

	return true;
}

ModelDescription Loader::GetDescription(const std::wstring &filename)
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
	m_sizeInfo = loadSizeInfo(cursor);
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

Loader::Header* Loader::loadHeader(const char *&data)
{
	Header *header = (Header*)data;
	data += sizeof Header;

	// Check file signature
	if (strncmp("Pmx ", header->abMagic, 4) != 0 && strncmp("PMX ", header->abMagic, 4) != 0)
		return nullptr;

	// Check supported versions
	if (header->fVersion != 2.0f && header->fVersion != 2.1f)
		return nullptr;

	return header;
}

Loader::SizeInfo* Loader::loadSizeInfo(const char *&data)
{
	SizeInfo *sizeInfo = (SizeInfo*)data;
	data += sizeof SizeInfo;

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
		readVector<float>(&vertex->uvEx[0].x, m_sizeInfo->cbUVVectorSize, data);
		vertex->weightMethod = readInfo<VertexWeightMethod>(data);
		std::memset(&vertex->boneInfo, 0, sizeof(vertex->boneInfo));
		switch (vertex->weightMethod)
		{
		case VertexWeightMethod::BDEF1:
			vertex->boneInfo.BDEF.boneIndexes[0] = readAsU32(m_sizeInfo->cbBoneIndexSize, data);
			vertex->boneInfo.BDEF.weights[0] = 1.0f;
			break;
		case VertexWeightMethod::BDEF2:
			for (i = 0; i < 2; i++)
				vertex->boneInfo.BDEF.boneIndexes[i] = readAsU32(m_sizeInfo->cbBoneIndexSize, data);

			vertex->boneInfo.BDEF.weights[0] = readInfo<float>(data);
			vertex->boneInfo.BDEF.weights[1] = 1.0f - vertex->boneInfo.BDEF.weights[0];
			break;
		case VertexWeightMethod::QDEF:
			if (m_header->fVersion < 2.1f)
				throw Exception("QDEF not supported on PMX version lower than 2.1");
		case VertexWeightMethod::BDEF4:
			for (i = 0; i < 4; i++)
				vertex->boneInfo.BDEF.boneIndexes[i] = readAsU32(m_sizeInfo->cbBoneIndexSize, data);
			readVector(vertex->boneInfo.BDEF.weights, 4, data);
			break;
		case VertexWeightMethod::SDEF:
			vertex->boneInfo.SDEF.boneIndexes[0] = readAsU32(m_sizeInfo->cbBoneIndexSize, data);
			vertex->boneInfo.SDEF.boneIndexes[1] = readAsU32(m_sizeInfo->cbBoneIndexSize, data);
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
	}
}

void Loader::loadIndexData(Model *model, const char *&data)
{
	model->verticesIndex.resize(readInfo<int>(data));

	for (auto &index : model->verticesIndex)
	{
		index = readAsU32(m_sizeInfo->cbVertexIndexSize, data);
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

		material->baseTexture = readAsU32(m_sizeInfo->cbMaterialIndexSize, data);
		material->sphereTexture = readAsU32(m_sizeInfo->cbMaterialIndexSize, data);
		material->sphereMode = readInfo<MaterialSphereMode>(data);
		material->toonFlag = readInfo<MaterialToonMode>(data);

		switch (material->toonFlag) {
		case MaterialToonMode::DefaultTexture:
			material->toonTexture.default = readInfo<uint8_t>(data);
			break;
		case MaterialToonMode::CustomTexture:
			material->toonTexture.custom = readAsU32(m_sizeInfo->cbTextureIndexSize, data);
			break;
		}

		material->freeField = getString(data);

		material->indexCount = readInfo<int>(data);
	}
}

void Loader::loadBones(Model *model, const char *&data)
{
	model->bones.resize(readInfo<int>(data));

	uint32_t id = 0;

	for (auto &bone : model->bones)
	{
		auto b = new detail::BoneImpl(model, id++);
		bone = b;
		readName(b->name, data);

		readVector<float>(b->startPosition, 3, data);
		b->m_parentId = readAsU32(m_sizeInfo->cbBoneIndexSize, data);
		b->m_deformationOrder = readInfo<int>(data);

		b->m_flags = readInfo<uint16_t>(data);

		if (b->m_flags & (uint16_t)BoneFlags::Attached)
			b->size.attachTo = readAsU32(m_sizeInfo->cbBoneIndexSize, data);
		else readVector<float>(b->size.length, 3, data);

		if (b->m_flags & ((uint16_t)BoneFlags::RotationAttached | (uint16_t)BoneFlags::TranslationAttached)) {
			b->inherit.from = readAsU32(m_sizeInfo->cbBoneIndexSize, data);
			b->inherit.rate = readInfo<float>(data);
		}
		else {
			b->inherit.from = 0xFFFFFFFFU;
			b->inherit.rate = 1.0f;
		}

		if (b->m_flags & (uint16_t)BoneFlags::FixedAxis) {
			readVector<float>(b->axisTranslation.m128_f32, 3, data);
		}

		if (b->m_flags & (uint16_t)BoneFlags::LocalAxis) {
			readVector<float>(b->localAxes.xDirection.m128_f32, 3, data);
			readVector<float>(b->localAxes.zDirection.m128_f32, 3, data);
		}

		if (b->m_flags & (uint16_t)BoneFlags::OuterParentDeformation) {
			b->externalDeformationKey = readInfo<int>(data);
		}

		if (b->m_flags & (uint16_t)BoneFlags::IK) {
			b->ik.begin = readAsU32(m_sizeInfo->cbBoneIndexSize, data);
			b->ik.loopCount = readInfo<int>(data);
			b->ik.angleLimit = readInfo<float>(data);

			b->ik.links.resize(readInfo<int>(data));
			for (auto &link : b->ik.links) {
				link.bone = readAsU32(m_sizeInfo->cbBoneIndexSize, data);
				link.limitAngle = readInfo<bool>(data);
				if (link.limitAngle) {
					readVector<float>(link.limits.lower.m128_f32, 3, data);
					readVector<float>(link.limits.upper.m128_f32, 3, data);
				}
			}
		}
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
				if (m_header->fVersion < 2.1f)
					throw Exception("Flip morph not available for PMX version < 2.1");
			case MorphType::Group:
				mdata.group.index = readAsU32(m_sizeInfo->cbMorphIndexSize, data);
				mdata.group.rate = readInfo<float>(data);
				break;
			case MorphType::Vertex:
				mdata.vertex.index = readAsU32(m_sizeInfo->cbVertexIndexSize, data);
				readVector<float>(mdata.vertex.offset, 3, data);
				break;
			case MorphType::Bone:
				mdata.bone.index = readAsU32(m_sizeInfo->cbVertexIndexSize, data);
				readVector<float>(mdata.bone.movement, 3, data);
				readVector<float>(mdata.bone.rotation, 4, data);
				break;
			case MorphType::UV:
			case MorphType::UV1:
			case MorphType::UV2:
			case MorphType::UV3:
			case MorphType::UV4:
				mdata.uv.index = readAsU32(m_sizeInfo->cbVertexIndexSize, data);
				readVector<float>(mdata.uv.offset, 4, data);
				break;
			case MorphType::Material:
				mdata.material.index = readAsU32(m_sizeInfo->cbMaterialIndexSize, data);
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
				if (m_header->fVersion < 2.1f)
					throw Exception("Impulse morph not supported in PMX version < 2.1");

				mdata.impulse.index = readAsU32(m_sizeInfo->cbRigidBodyIndexSize, data);
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
				morph.id = readAsU32(m_sizeInfo->cbBoneIndexSize, data);
				break;
			case FrameMorphTarget::Morph:
				morph.id = readAsU32(m_sizeInfo->cbMorphIndexSize, data);
				break;
			}
		}
	}
}

void Loader::loadRigidBodies(Model *model, const char *&data)
{
	model->bodies.resize(readInfo<int>(data));

	for (auto &body : model->bodies)
	{
		body = new RigidBody;
		readName(body->name, data);

		body->targetBone = readAsU32(m_sizeInfo->cbBoneIndexSize, data);

		body->group = readInfo<uint8_t>(data);
		body->groupMask = readInfo<uint16_t>(data);

		body->shape = readInfo<PMX::RigidBodyShape>(data);
		readVector<float>(&body->size.x, 3, data);

		readVector<float>(&body->position.x, 3, data);
		readVector<float>(&body->rotation.x, 3, data);

		body->mass = readInfo<float>(data);

		body->linearDamping = readInfo<float>(data);
		body->angularDamping = readInfo<float>(data);
		body->restitution = readInfo<float>(data);
		body->friction = readInfo<float>(data);

		body->mode = readInfo<PMX::RigidBodyMode>(data);
	}
}

void Loader::loadJoints(Model *model, const char *&data)
{
	model->joints.resize(readInfo<int>(data));

	for (auto &joint : model->joints)
	{
		joint = new Joint;
		readName(joint->name, data);

		joint->type = readInfo<JointType>(data);

		if (m_header->fVersion < 2.1f && joint->type != JointType::Spring6DoF)
			throw Exception("Invalid joint type for PMX version < 2.1");

		joint->data.bodyA = readAsU32(m_sizeInfo->cbRigidBodyIndexSize, data);
		joint->data.bodyB = readAsU32(m_sizeInfo->cbRigidBodyIndexSize, data);

		readVector<float>(&joint->data.position.x, 3, data);
		readVector<float>(&joint->data.rotation.x, 3, data);

		readVector<float>(&joint->data.lowerMovementRestrictions.x, 3, data);
		readVector<float>(&joint->data.upperMovementRestrictions.x, 3, data);
		readVector<float>(&joint->data.lowerRotationRestrictions.x, 3, data);
		readVector<float>(&joint->data.upperRotationRestrictions.x, 3, data);

		readVector<float>(joint->data.springConstant, 6, data);
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
		body->material = readAsU32(m_sizeInfo->cbMaterialIndexSize, data);

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
			anchor.rigidBodyIndex = readAsU32(m_sizeInfo->cbRigidBodyIndexSize, data);
			anchor.vertexIndex = readAsU32(m_sizeInfo->cbVertexIndexSize, data);
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

	switch (m_sizeInfo->cbEncoding) {
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
