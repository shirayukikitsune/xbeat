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

#include "PMXLoader.h"

#include <algorithm>
#include <codecvt>

#include <AnimatedModel.h>
#include <Context.h>
#include <Geometry.h>
#include <IndexBuffer.h>
#include <Material.h>
#include <Model.h>
#include <ResourceCache.h>
#include <Skeleton.h>
#include <VertexBuffer.h>

using namespace std;
using namespace PMX;

bool Loader::loadFromFile(Urho3D::AnimatedModel* Model, const wstring &Filename)
{
	ifstream Ifile;
	Ifile.open(Filename, ios::binary);
	if (!Ifile.good())
		return false;

	bool Ret = loadFromStream(Model, Ifile);
	Ifile.close();
	return Ret;
}

bool Loader::loadFromStream(Urho3D::AnimatedModel* Model, istream &In)
{
	istream::pos_type Pos = In.tellg();
	In.seekg(0, In.end);
	istream::pos_type Len = In.tellg();
	Len -= Pos;
	In.seekg(Pos, In.beg);

	char *Data = new char[Len];
	const char *Cursor = Data;

	In.read(Data, Len);
	bool Ret = loadFromMemory(Model, Cursor);
	In.seekg(Pos + (istream::pos_type)(Cursor - Data), In.beg);

	delete[] Data;

	return Ret;
}

bool Loader::loadFromMemory(Urho3D::AnimatedModel* AnimatedModel, const char *&Data)
{
	Header = loadHeader(Data);
	// Check if we have a valid header
	if (Header == nullptr)
		return false;

	SizeInfo = loadSizeInfo(Data);

	ModelDescription Description;
	loadDescription(Description, Data);

	vector<Vertex> VertexList;
	loadVertexData(VertexList, Data);

	vector<uint32_t> IndexList;
	loadIndexData(IndexList, Data);

	vector<std::wstring> TextureList;
	loadTextures(TextureList, Data);

	vector<Material> MaterialList;
	loadMaterials(MaterialList, Data);

	vector<Bone> BoneList;
	loadBones(BoneList, Data);

	vector<Morph> MorphList;
	loadMorphs(MorphList, Data);

	vector<Frame> FrameList;
	loadFrames(FrameList, Data);

	vector<RigidBody> RigidBodyList;
	loadRigidBodies(RigidBodyList, Data);

	vector<Joint> JointList;
	loadJoints(JointList, Data);

	vector<SoftBody> SoftBodyList;
	if (Header->Version >= 2.1f)
		loadSoftBodies(SoftBodyList, Data);

	// Now parse the model data
	using namespace Urho3D;

	ResourceCache* Cache = AnimatedModel->GetContext()->GetSubsystem<ResourceCache>();
	SharedPtr<Urho3D::Model> Model(new Urho3D::Model(AnimatedModel->GetContext()));

	Model->SetNumGeometries(MaterialList.size());

	vector<Vector3> Positions;

	uint32_t lastIndex = 0;
	for (size_t MaterialIndex = 0; MaterialIndex < MaterialList.size(); ++MaterialIndex) {
		SharedPtr<IndexBuffer> IBuffer(new IndexBuffer(Model->GetContext()));
		SharedPtr<VertexBuffer> VBuffer(new VertexBuffer(Model->GetContext()));

		auto& ModelMat = MaterialList[MaterialIndex];

		IBuffer->SetShadowed(true);
		IBuffer->SetSize((unsigned int)ModelMat.indexCount, true, true);

		VBuffer->SetShadowed(true);
		VBuffer->SetSize((unsigned int)ModelMat.indexCount, MASK_POSITION | MASK_NORMAL | MASK_TEXCOORD1 | MASK_BLENDWEIGHTS | MASK_BLENDINDICES, true);

		uint32_t *ibData = new uint32_t[ModelMat.indexCount];
		uint32_t *pData = ibData;
		std::memset(ibData, 0, sizeof(uint32_t) * ModelMat.indexCount);
		unsigned char *data = new unsigned char[VBuffer->GetVertexSize() * ModelMat.indexCount];
		std::memset(data, 0, VBuffer->GetVertexSize() * ModelMat.indexCount);
		float* pd = (float*)data;

		Vector3 Center = Vector3::ZERO;
		for (size_t index = 0; index < ModelMat.indexCount; ++index)
		{
			*pData++ = index;
			auto &vertex = VertexList[IndexList[index + lastIndex]];
			Positions.emplace_back(vertex.position);
			Center += Positions[index] / (float)ModelMat.indexCount;
			*pd++ = vertex.position[0];
			*pd++ = vertex.position[1];
			*pd++ = vertex.position[2];
			*pd++ = vertex.normal[0];
			*pd++ = vertex.normal[1];
			*pd++ = vertex.normal[2];
			*pd++ = vertex.uv[0];
			*pd++ = vertex.uv[1];
			unsigned char *indices = (unsigned char*)(pd + 4);
			switch (vertex.weightMethod) {
			case VertexWeightMethod::BDEF4:
			case VertexWeightMethod::QDEF:
				*(pd + 2) = vertex.boneInfo.BDEF.weights[2];
				*(pd + 3) = vertex.boneInfo.BDEF.weights[3];
				*(indices + 2) = (unsigned char)vertex.boneInfo.BDEF.boneIndexes[2];
				*(indices + 3) = (unsigned char)vertex.boneInfo.BDEF.boneIndexes[3];
			case VertexWeightMethod::BDEF2:
				*(pd + 1) = vertex.boneInfo.BDEF.weights[1];
				*(indices + 1) = (unsigned char)vertex.boneInfo.BDEF.boneIndexes[1];
			case VertexWeightMethod::BDEF1:
				*(pd) = vertex.boneInfo.BDEF.weights[0];
				*(indices) = (unsigned char)vertex.boneInfo.BDEF.boneIndexes[0];
				break;
			case VertexWeightMethod::SDEF:
				*(pd) = vertex.boneInfo.SDEF.weightBias;
				*(pd + 1) = 1.0f - vertex.boneInfo.SDEF.weightBias;
				*(indices) = (unsigned char)vertex.boneInfo.SDEF.boneIndexes[0];
				*(indices + 1) = (unsigned char)vertex.boneInfo.SDEF.boneIndexes[1];
			}
			pd += 5;
		}
		IBuffer->SetData(ibData);
		VBuffer->SetData(data);

		delete[] ibData;
		delete[] data;
		
		lastIndex += ModelMat.indexCount;

		SharedPtr<Geometry> Geom(new Geometry(Model->GetContext()));
		Geom->SetVertexBuffer(0, VBuffer);
		Geom->SetIndexBuffer(IBuffer);
		Geom->SetDrawRange(TRIANGLE_LIST, 0, ModelMat.indexCount, 0, ModelMat.indexCount, false);

		Model->SetGeometry(MaterialIndex, 0, Geom);
		Model->SetGeometryCenter(MaterialIndex, Center);
	}

	/*std::vector<Frame>::iterator frame;
	for (frame = FrameList.begin(); frame != FrameList.end(); ++frame) {
		if (frame->type == 1 && frame->name.japanese.compare(L"Root") == 0)
			break;
	}

	std::vector<PMX::Loader::Bone>::iterator root;
	for (root = BoneList.begin(); root != BoneList.end(); ++root) {
		if (root->index == frame->morphs.front().id)
			break;
	}*/


	Skeleton Skeleton;
	auto& Bones = Skeleton.GetModifiableBones();
	Bones.Reserve((unsigned int)BoneList.size());

	for (unsigned int BoneIndex = 0; BoneIndex != BoneList.size(); ++BoneIndex)
	{
		Bone& Bone = BoneList[BoneIndex];
		Urho3D::Bone NewBone;
		NewBone.name_ = Bone.Name.japanese.c_str();
		NewBone.initialPosition_ = Vector3(Bone.InitialPosition);
		NewBone.initialScale_ = Vector3(1.0f, 1.0f, 1.0f);
		NewBone.collisionMask_ = BONECOLLISION_BOX | BONECOLLISION_SPHERE;
		NewBone.parentIndex_ = Bone.Parent == -1 ? BoneIndex : Bone.Parent;
		Bones.Push(NewBone);
	}
	Skeleton.SetRootBoneIndex(0);
	Model->SetSkeleton(Skeleton);
	AnimatedModel->SetModel(Model, false);

	Model->SetBoundingBox(BoundingBox(Positions.data(), (uint32_t)Positions.size()));

	return true;
}

ModelDescription Loader::getDescription(const wstring &Filename)
{
	ifstream Ifile;
	Ifile.open(Filename, ios::binary);
	if (!Ifile.good())
		throw Exception("Unable to open the requested filename");

	Ifile.seekg(0, Ifile.end);
	istream::pos_type Len = Ifile.tellg();
	Ifile.seekg(0, Ifile.beg);

	char *Data = new char[Len];
	const char *cursor = Data;

	Ifile.read(Data, Len);

	auto Header = loadHeader(cursor);
	if (Header == nullptr) throw Exception("Unrecognized file format");
	SizeInfo = loadSizeInfo(cursor);
	ModelDescription Output;
	loadDescription(Output, cursor);

	delete[] Data;
	Ifile.close();

	return Output;
}

template <class T>
void readInfo(T& Value, const char *&Data)
{
	memcpy(&Value, Data, sizeof(T));
	Data += sizeof(T);
}
template <class T>
T readInfo(const char *&Data)
{
	T Value;
	memcpy(&Value, Data, sizeof(T));
	Data += sizeof(T);

	return Value;
}
template <class T>
void readVector(T* Vec, size_t Size, const char *&Data)
{
	for (size_t s = 0; s < Size; s++) {
		readInfo<T>(Vec[s], Data);
	}
}

Loader::FileHeader* Loader::loadHeader(const char *&Data)
{
	FileHeader *Header = (FileHeader*)Data;
	Data += sizeof FileHeader;

	// Check file signature
	if (strncmp("Pmx ", Header->Magic, 4) != 0 && strncmp("PMX ", Header->Magic, 4) != 0)
		return nullptr;

	// Check supported versions
	if (Header->Version != 2.0f && Header->Version != 2.1f)
		return nullptr;

	return Header;
}

Loader::FileSizeInfo* Loader::loadSizeInfo(const char *&Data)
{
	FileSizeInfo *SizeInfo = (FileSizeInfo*)Data;
	Data += sizeof FileSizeInfo;

	return SizeInfo;
}

void Loader::loadDescription(ModelDescription &Desc, const char *&Data)
{
	readName(Desc.name, Data);
	readName(Desc.comment, Data);
}

void Loader::loadVertexData(std::vector<Vertex> &VertexList, const char*& Data) {
	VertexList.resize(readInfo<int>(Data));
	int i;
	uint32_t id = 0;

	for (auto &vertex : VertexList)
	{
		readVector<float>(vertex.position, 3, Data);
		readVector<float>(vertex.normal, 3, Data);
		readVector<float>(vertex.uv, 2, Data);
		for (int j = 0; j < SizeInfo->UVVectorSize; ++j)
			readVector<float>(vertex.uvEx[j], 4, Data);
		vertex.weightMethod = readInfo<VertexWeightMethod>(Data);
		memset(&vertex.boneInfo, 0, sizeof(vertex.boneInfo));
		switch (vertex.weightMethod)
		{
		case VertexWeightMethod::BDEF1:
			vertex.boneInfo.BDEF.boneIndexes[0] = readAsU32(SizeInfo->BoneIndexSize, Data);
			vertex.boneInfo.BDEF.weights[0] = 1.0f;
			break;
		case VertexWeightMethod::BDEF2:
			for (i = 0; i < 2; i++)
				vertex.boneInfo.BDEF.boneIndexes[i] = readAsU32(SizeInfo->BoneIndexSize, Data);

			vertex.boneInfo.BDEF.weights[0] = readInfo<float>(Data);
			vertex.boneInfo.BDEF.weights[1] = 1.0f - vertex.boneInfo.BDEF.weights[0];
			break;
		case VertexWeightMethod::QDEF:
			if (Header->Version < 2.1f)
				throw Exception("QDEF not supported on PMX version lower than 2.1");
		case VertexWeightMethod::BDEF4:
			for (i = 0; i < 4; i++)
				vertex.boneInfo.BDEF.boneIndexes[i] = readAsU32(SizeInfo->BoneIndexSize, Data);
			readVector(vertex.boneInfo.BDEF.weights, 4, Data);
			break;
		case VertexWeightMethod::SDEF:
			vertex.boneInfo.SDEF.boneIndexes[0] = readAsU32(SizeInfo->BoneIndexSize, Data);
			vertex.boneInfo.SDEF.boneIndexes[1] = readAsU32(SizeInfo->BoneIndexSize, Data);
			vertex.boneInfo.SDEF.weightBias = readInfo<float>(Data);
			readVector<float>(vertex.boneInfo.SDEF.C, 3, Data);
			readVector<float>(vertex.boneInfo.SDEF.R0, 3, Data);
			readVector<float>(vertex.boneInfo.SDEF.R1, 3, Data);
			break;
		default:
			throw Exception("Invalid value for vertex weight method");
		}
		vertex.edgeWeight = readInfo<float>(Data);

		vertex.index = id++;
	}
}

void Loader::loadIndexData(std::vector<uint32_t> &IndexList, const char *&Data)
{
	IndexList.resize(readInfo<int>(Data));

	for (auto &Index : IndexList)
	{
		Index = readAsU32(SizeInfo->VertexIndexSize, Data);
	}
}

void Loader::loadTextures(std::vector<std::wstring> &TextureList, const char *&Data)
{
	TextureList.resize(readInfo<int>(Data));

	for (auto &Texture : TextureList)
	{
		Texture = getString(Data);
	}
}

void Loader::loadMaterials(std::vector<Material> &MaterialList, const char *&Data)
{
	MaterialList.resize(readInfo<int>(Data));

	for (auto &material : MaterialList)
	{
		readName(material.name, Data);

		readVector<float>(material.diffuse, 4, Data);
		readVector<float>(material.specular, 3, Data);
		material.specularCoefficient = readInfo<float>(Data);
		readVector<float>(material.ambient, 3, Data);

		material.flags = readInfo<uint8_t>(Data);
		
		readVector<float>(material.edgeColor, 4, Data);
		material.edgeSize = readInfo<float>(Data);

		material.baseTexture = readAsU32(SizeInfo->MaterialIndexSize, Data);
		material.sphereTexture = readAsU32(SizeInfo->MaterialIndexSize, Data);
		material.sphereMode = readInfo<MaterialSphereMode>(Data);
		material.toonFlag = readInfo<MaterialToonMode>(Data);

		switch (material.toonFlag) {
		case MaterialToonMode::DefaultTexture:
			material.toonTexture.default = readInfo<uint8_t>(Data);
			break;
		case MaterialToonMode::CustomTexture:
			material.toonTexture.custom = readAsU32(SizeInfo->TextureIndexSize, Data);
			break;
		}

		material.freeField = getString(Data);

		material.indexCount = readInfo<int>(Data);
	}
}

void Loader::loadBones(std::vector<Bone> &BoneList, const char *&Data)
{
	BoneList.resize(readInfo<int>(Data));

	uint32_t id = 0;

	for (auto &Bone : BoneList)
	{
		readName(Bone.Name, Data);

		readVector<float>(Bone.InitialPosition, 3, Data);
		Bone.Parent = readAsU32(SizeInfo->BoneIndexSize, Data);
		Bone.DeformationOrder = readInfo<int>(Data);
		Bone.index = id++;

		Bone.Flags = readInfo<uint16_t>(Data);

		if (Bone.Flags & (uint16_t)BoneFlags::Attached)
			Bone.Size.AttachTo = readAsU32(SizeInfo->BoneIndexSize, Data);
		else readVector<float>(Bone.Size.Length, 3, Data);

		if (Bone.Flags & ((uint16_t)BoneFlags::RotationAttached | (uint16_t)BoneFlags::TranslationAttached)) {
			Bone.Inherit.From = readAsU32(SizeInfo->BoneIndexSize, Data);
			Bone.Inherit.Rate = readInfo<float>(Data);
		}
		else {
			Bone.Inherit.From = 0xFFFFFFFFU;
			Bone.Inherit.Rate = 1.0f;
		}

		if (Bone.Flags & (uint16_t)BoneFlags::FixedAxis) {
			readVector<float>(Bone.AxisTranslation, 3, Data);
		}

		if (Bone.Flags & (uint16_t)BoneFlags::LocalAxis) {
			readVector<float>(Bone.LocalAxes.X, 3, Data);
			readVector<float>(Bone.LocalAxes.Z, 3, Data);
		}

		if (Bone.Flags & (uint16_t)BoneFlags::OuterParentDeformation) {
			Bone.ExternalDeformationKey = readInfo<int>(Data);
		}

		if (Bone.Flags & (uint16_t)BoneFlags::IK) {
			Bone.IkData.targetIndex = readAsU32(SizeInfo->BoneIndexSize, Data);
			Bone.IkData.loopCount = readInfo<int>(Data);
			Bone.IkData.angleLimit = readInfo<float>(Data);

			Bone.IkData.links.resize(readInfo<int>(Data));
			for (auto &Link : Bone.IkData.links) {
				Link.boneIndex = readAsU32(SizeInfo->BoneIndexSize, Data);
				Link.limitAngle = readInfo<bool>(Data);
				if (Link.limitAngle) {
					readVector<float>(Link.limits.lower, 3, Data);
					readVector<float>(Link.limits.upper, 3, Data);
				}
			}
		}
	}
}

void Loader::loadMorphs(std::vector<Morph> &MorphList, const char *&Data)
{
	MorphList.resize(readInfo<int>(Data));
	
	for (auto &morph : MorphList)
	{
		readName(morph.name, Data);

		morph.operation = readInfo<uint8_t>(Data);
		morph.type = readInfo<uint8_t>(Data);
		morph.data.resize(readInfo<int>(Data));

		for (auto &mdata : morph.data)
		{
			switch (morph.type)
			{
			case MorphType::Flip:
				if (Header->Version < 2.1f)
					throw Exception("Flip morph not available for PMX version < 2.1");
			case MorphType::Group:
				mdata.group.index = readAsU32(SizeInfo->MorphIndexSize, Data);
				mdata.group.rate = readInfo<float>(Data);
				break;
			case MorphType::Vertex:
				mdata.vertex.index = readAsU32(SizeInfo->VertexIndexSize, Data);
				readVector<float>(mdata.vertex.offset, 3, Data);
				break;
			case MorphType::Bone:
				mdata.bone.index = readAsU32(SizeInfo->VertexIndexSize, Data);
				readVector<float>(mdata.bone.movement, 3, Data);
				readVector<float>(mdata.bone.rotation, 4, Data);
				break;
			case MorphType::UV:
			case MorphType::UV1:
			case MorphType::UV2:
			case MorphType::UV3:
			case MorphType::UV4:
				mdata.uv.index = readAsU32(SizeInfo->VertexIndexSize, Data);
				readVector<float>(mdata.uv.offset, 4, Data);
				break;
			case MorphType::Material:
				mdata.material.index = readAsU32(SizeInfo->MaterialIndexSize, Data);
				mdata.material.method = readInfo<MaterialMorphMethod>(Data);
				readVector<float>(mdata.material.diffuse, 4, Data);
				readVector<float>(mdata.material.specular, 3, Data);
				mdata.material.specularCoefficient = readInfo<float>(Data);
				readVector<float>(mdata.material.ambient, 3, Data);
				readVector<float>(mdata.material.edgeColor, 4, Data);
				mdata.material.edgeSize = readInfo<float>(Data);
				readVector<float>(mdata.material.baseCoefficient, 4, Data);
				readVector<float>(mdata.material.sphereCoefficient, 4, Data);
				readVector<float>(mdata.material.toonCoefficient, 4, Data);
				break;
			case MorphType::Impulse:
				if (Header->Version < 2.1f)
					throw Exception("Impulse morph not supported in PMX version < 2.1");

				mdata.impulse.index = readAsU32(SizeInfo->RigidBodyIndexSize, Data);
				mdata.impulse.localFlag = readInfo<uint8_t>(Data);
				readVector<float>(mdata.impulse.velocity, 3, Data);
				readVector<float>(mdata.impulse.rotationTorque, 3, Data);
				break;
			default:
				throw Exception("Invalid morph type");
			}
		}
	}
}

void Loader::loadFrames(std::vector<Frame> &FrameList, const char *&Data)
{
	FrameList.resize(readInfo<int>(Data));

	for (auto &frame : FrameList)
	{
		readName(frame.name, Data);
		frame.type = readInfo<uint8_t>(Data);
		frame.morphs.resize(readInfo<int>(Data));

		for (auto &morph : frame.morphs)
		{
			morph.target = readInfo<FrameMorphTarget>(Data);
			switch (morph.target) {
			case FrameMorphTarget::Bone:
				morph.id = readAsU32(SizeInfo->BoneIndexSize, Data);
				break;
			case FrameMorphTarget::Morph:
				morph.id = readAsU32(SizeInfo->MorphIndexSize, Data);
				break;
			}
		}
	}
}

void Loader::loadRigidBodies(std::vector<RigidBody> &RigidBodyList, const char *&Data)
{
	RigidBodyList.resize(readInfo<int>(Data));

	for (auto &Body : RigidBodyList)
	{
		readName(Body.name, Data);

		Body.targetBone = readAsU32(SizeInfo->BoneIndexSize, Data);

		Body.group = readInfo<uint8_t>(Data);
		Body.groupMask = readInfo<uint16_t>(Data);

		Body.shape = readInfo<PMX::RigidBodyShape>(Data);
		readVector<float>(Body.size, 3, Data);

		readVector<float>(Body.position, 3, Data);
		readVector<float>(Body.rotation, 3, Data);

		Body.mass = readInfo<float>(Data);

		Body.linearDamping = readInfo<float>(Data);
		Body.angularDamping = readInfo<float>(Data);
		Body.restitution = readInfo<float>(Data);
		Body.friction = readInfo<float>(Data);

		Body.mode = readInfo<PMX::RigidBodyMode>(Data);
	}
}

void Loader::loadJoints(std::vector<Joint> &JointList, const char *&Data)
{
	JointList.resize(readInfo<int>(Data));

	for (auto &Joint : JointList)
	{
		readName(Joint.name, Data);

		Joint.type = readInfo<JointType>(Data);

		if (Header->Version < 2.1f && Joint.type != JointType::Spring6DoF)
			throw Exception("Invalid joint type for PMX version < 2.1");

		Joint.data.bodyA = readAsU32(SizeInfo->RigidBodyIndexSize, Data);
		Joint.data.bodyB = readAsU32(SizeInfo->RigidBodyIndexSize, Data);

		readVector<float>(Joint.data.position, 3, Data);
		readVector<float>(Joint.data.rotation, 3, Data);

		readVector<float>(Joint.data.lowerMovementRestrictions, 3, Data);
		readVector<float>(Joint.data.upperMovementRestrictions, 3, Data);
		readVector<float>(Joint.data.lowerRotationRestrictions, 3, Data);
		readVector<float>(Joint.data.upperRotationRestrictions, 3, Data);

		readVector<float>(Joint.data.springConstant, 6, Data);
	}
}

void Loader::loadSoftBodies(std::vector<SoftBody> &SoftBodyList, const char *&Data)
{
	SoftBodyList.resize(readInfo<int>(Data));

	for (auto &body : SoftBodyList)
	{
		readName(body.name, Data);

		body.shape = readInfo<SoftBody::Shape::Shape_e>(Data);
		body.material = readAsU32(SizeInfo->MaterialIndexSize, Data);

		body.group = readInfo<uint8_t>(Data);
		body.groupFlags = readInfo<uint16_t>(Data);

		body.flags = readInfo<SoftBody::Flags::Flags_e>(Data);

		body.blinkCreationDistance = readInfo<int>(Data);
		body.clusterCount = readInfo<int>(Data);

		body.mass = readInfo<float>(Data);
		body.collisionMargin = readInfo<float>(Data);

		body.model = readInfo<SoftBody::AeroModel::AeroModel_e>(Data);

		readInfo<SoftBody::Config>(body.config, Data);
		readInfo<SoftBody::Cluster>(body.cluster, Data);
		readInfo<SoftBody::Iteration>(body.iteration, Data);
		readInfo<SoftBody::Material>(body.materialInfo, Data);

		body.anchors.resize(readInfo<int>(Data));
		for (auto &anchor : body.anchors)
		{
			anchor.rigidBodyIndex = readAsU32(SizeInfo->RigidBodyIndexSize, Data);
			anchor.vertexIndex = readAsU32(SizeInfo->VertexIndexSize, Data);
			anchor.nearMode = readInfo<uint8_t>(Data);
		}

		int count = readInfo<int>(Data);
		body.pins.resize(count);
		readVector<SoftBody::Pin>(body.pins.data(), count, Data);
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

wstring Loader::getString(const char *&data) {
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
