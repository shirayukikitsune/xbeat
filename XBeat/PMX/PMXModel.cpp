#include "PMXModel.h"

#include <codecvt>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/VertexBuffer.h>

using namespace std;
using namespace Urho3D;

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

PMXModel::PMXModel(Context *context)
	: Model(context)
{
}

PMXModel::~PMXModel()
{
}

void PMXModel::RegisterObject(Context* context)
{
	context->RegisterFactory<PMXModel>();
}

bool PMXModel::BeginLoad(Deserializer& source)
{
	// Check if we have a valid header
	if (!LoadHeader(header, source))
		return false;

	bool async = GetAsyncLoadState() == ASYNC_LOADING;

	LoadSizeInfo(sizeInfo, source);
	LoadDescription(description, source);

	LoadVertexData(vertexList, source);
	LoadIndexData(indexList, source);
	LoadTextures(textureList, source);
	LoadMaterials(materialList, source);
	LoadBones(boneList, source);
	LoadMorphs(morphList, source);
	LoadFrames(frameList, source);
	LoadRigidBodies(rigidBodyList, source);
	LoadJoints(jointList, source);

	if (header.Version >= 2.1f)
		LoadSoftBodies(softBodyList, source);

	return true;
}

bool PMXModel::EndLoad()
{
	this->SetNumGeometries(materialList.Size());

	PODVector<Vector3> positions;
	Vector<PODVector<unsigned int>> allBoneMappings;
	Vector<SharedPtr<IndexBuffer>> iBuffers;
	Vector<SharedPtr<VertexBuffer>> vBuffers;

	allBoneMappings.Reserve(materialList.Size());
	iBuffers.Reserve(materialList.Size());
	vBuffers.Reserve(materialList.Size());

	unsigned int lastIndex = 0;
	for (size_t materialIndex = 0; materialIndex < materialList.Size(); ++materialIndex) {
		SharedPtr<IndexBuffer> iBuffer(new IndexBuffer(context_));
		SharedPtr<VertexBuffer> vBuffer(new VertexBuffer(context_));
		PODVector<unsigned int> boneMappings;
		HashMap<unsigned int, unsigned int> mapping;

		auto& modelMat = materialList[materialIndex];

		iBuffer->SetShadowed(true);
		iBuffer->SetSize((unsigned int)modelMat.indexCount, true, true);

		vBuffer->SetShadowed(true);
		vBuffer->SetSize((unsigned int)modelMat.indexCount, MASK_POSITION | MASK_NORMAL | MASK_TEXCOORD1 | MASK_BLENDWEIGHTS | MASK_BLENDINDICES, true);

		unsigned int *ibData = new unsigned int[modelMat.indexCount];
		unsigned int *pData = ibData;
		std::memset(ibData, 0, sizeof(unsigned int) * modelMat.indexCount);
		unsigned char *data = new unsigned char[vBuffer->GetVertexSize() * modelMat.indexCount];
		std::memset(data, 0, vBuffer->GetVertexSize() * modelMat.indexCount);
		float* pd = (float*)data;

		unsigned int freeBone = 0;

		auto mapBone = [&mapping, &boneMappings, &freeBone](unsigned int index) -> unsigned char {
			auto map = mapping.Find(index);

			if (map == mapping.End()) {
				mapping[index] = freeBone;
				boneMappings.Push(index);
				return (unsigned char)freeBone++;
			}
			else {
				return (unsigned char)map->second_;
			}
		};

		Vector3 center = Vector3::ZERO;
		for (size_t index = 0; index < modelMat.indexCount; ++index)
		{
			*pData++ = index;
			auto &vertex = vertexList[indexList[index + lastIndex]];

			positions.Push(vertex.position);
			center += positions.Back() / (float)modelMat.indexCount;
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
			case PMX::VertexWeightMethod::BDEF4:
			case PMX::VertexWeightMethod::QDEF:
				*(pd + 2) = vertex.boneInfo.BDEF.weights[2];
				*(pd + 3) = vertex.boneInfo.BDEF.weights[3];
				*(indices + 2) = mapBone(vertex.boneInfo.BDEF.boneIndexes[2]);
				*(indices + 3) = mapBone(vertex.boneInfo.BDEF.boneIndexes[3]);
			case PMX::VertexWeightMethod::BDEF2:
				*(pd + 1) = vertex.boneInfo.BDEF.weights[1];
				*(indices + 1) = mapBone(vertex.boneInfo.BDEF.boneIndexes[1]);
			case PMX::VertexWeightMethod::BDEF1:
				*(pd) = vertex.boneInfo.BDEF.weights[0];
				*(indices) = mapBone(vertex.boneInfo.BDEF.boneIndexes[0]);
				break;
			case PMX::VertexWeightMethod::SDEF:
				*(pd) = vertex.boneInfo.SDEF.weightBias;
				*(pd + 1) = 1.0f - vertex.boneInfo.SDEF.weightBias;
				*(indices) = mapBone(vertex.boneInfo.SDEF.boneIndexes[0]);
				*(indices + 1) = mapBone(vertex.boneInfo.SDEF.boneIndexes[1]);
			}
			pd += 5;
		}
		iBuffer->SetData(ibData);
		vBuffer->SetData(data);

		delete[] ibData;
		delete[] data;

		lastIndex += modelMat.indexCount;

		SharedPtr<Geometry> geometry(new Geometry(context_));
		geometry->SetVertexBuffer(0, vBuffer);
		geometry->SetIndexBuffer(iBuffer);
		geometry->SetDrawRange(TRIANGLE_LIST, 0, modelMat.indexCount);

		SetGeometry(materialIndex, 0, geometry);
		SetGeometryCenter(materialIndex, center);

		allBoneMappings.Push(boneMappings);
		iBuffers.Push(iBuffer);
		vBuffers.Push(vBuffer);
	}

	// Find root bone from root frame
	Vector<PMX::Frame>::Iterator frame;
	for (frame = frameList.Begin(); frame != frameList.End(); ++frame) {
		if (frame->type == 1 && frame->name.japanese.Compare(L"Root") == 0) {
			if (frame->morphs.Size() == 0) {
				frame = frameList.End();
			}
			break;
		}
	}

	Vector<PMX::Bone>::Iterator rootIterator = boneList.End();
	if (frame != frameList.End()) {
		for (rootIterator = boneList.Begin(); rootIterator != boneList.End(); ++rootIterator) {
			if (rootIterator->index == frame->morphs.Front().id)
				break;
		}
	}
	PMX::Bone& root = (rootIterator == boneList.End() ? boneList[0] : boneList[rootIterator->index]);

	// Create skeleton
	Skeleton skeleton;
	auto& bones = skeleton.GetModifiableBones();
	bones.Reserve(boneList.Size());

	for (unsigned int boneIndex = 0; boneIndex != boneList.Size(); ++boneIndex) {
		PMX::Bone* parent = nullptr;
		PMX::Bone& bone = boneList[boneIndex];
		Urho3D::Bone newBone;

		newBone.initialPosition_ = Vector3::ZERO;
		newBone.initialRotation_ = Quaternion::IDENTITY;

		if (bone.index != root.index) {
			if (bone.Parent != -1) {
				parent = &boneList[bone.Parent];
				newBone.initialPosition_ = Vector3(bone.InitialPosition) - Vector3(boneList[bone.Parent].InitialPosition); 

				auto getDirection = [this](PMX::Bone &bone) {
					Vector3 direction = Vector3::ZERO;

					if (bone.Flags & PMX::BoneFlags::Attached) {
						if (bone.Size.AttachTo != -1) {
							PMX::Bone &attached = this->boneList[bone.Size.AttachTo];
							direction = (Vector3(attached.InitialPosition) - Vector3(bone.InitialPosition)).Normalized();
						}
					}
					else direction = Vector3(bone.Size.Length).Normalized();

					return direction;
				};

				Vector3 boneDirection = getDirection(bone), parentDirection = getDirection(*parent);

#if 0
				Quaternion worldRotation;
				worldRotation.FromLookRotation(boneDirection);
				newBone.initialRotation_.FromRotationTo(parentDirection, boneDirection);
#else
				newBone.offsetMatrix_.SetTranslation(-Vector3(bone.InitialPosition));
				auto axis = boneDirection.CrossProduct(parentDirection);
				if (axis.LengthSquared() > 0.0000001f) {
					axis.Normalize();
					float angle = boneDirection.DotProduct(parentDirection);
					if (fabsf(angle) > 0.000001f) {
						angle = acosf(angle);
						newBone.initialRotation_.FromAngleAxis(angle, axis);
						//newBone.offsetMatrix_ = Matrix3x4(Vector3(bone.InitialPosition), Quaternion(angle, axis), 1.0f).Inverse();
					}
				}
#endif
			}
			else {
				newBone.initialPosition_ = Vector3(bone.InitialPosition);
			}
		}

		newBone.initialScale_ = Vector3::ONE;
		newBone.name_ = bone.Name.japanese;
		newBone.nameHash_ = newBone.name_;
		newBone.parentIndex_ = bone.Parent == -1 ? root.index : bone.Parent;
		bones.Push(newBone);
	}

	skeleton.SetRootBoneIndex(root.index);
	this->SetSkeleton(skeleton);

	SetGeometryBoneMappings(allBoneMappings);
	SetIndexBuffers(iBuffers);
	PODVector<unsigned int> empty;
	SetVertexBuffers(vBuffers, empty, empty);
	SetName(description.name.japanese);
	SetBoundingBox(BoundingBox(&positions.At(0), positions.Size()));

	return true;
}

bool PMXModel::LoadHeader(PMXModel::FileHeader& header, Urho3D::Deserializer& source)
{
	auto readBytes = source.Read(&header, sizeof(PMXModel::FileHeader));
	if (readBytes != sizeof(PMXModel::FileHeader))
		return false;

	// Check file signature
	if (strncmp("Pmx ", header.Magic, 4) != 0 && strncmp("PMX ", header.Magic, 4) != 0)
		return false;

	// Check supported versions
	if (header.Version != 2.0f && header.Version != 2.1f)
		return false;

	return true;
}

void PMXModel::LoadSizeInfo(PMXModel::FileSizeInfo& sizeInfo, Urho3D::Deserializer& source)
{
	source.Read(&sizeInfo, sizeof(PMXModel::FileSizeInfo));
}

void PMXModel::LoadDescription(PMX::ModelDescription& description, Urho3D::Deserializer& source)
{
	readName(description.name, source);
	readName(description.comment, source);
}

void PMXModel::LoadVertexData(Urho3D::Vector<PMX::Vertex>& vertexList, Urho3D::Deserializer& source) {
	vertexList.Resize(source.ReadUInt());
	int i;
	unsigned int id = 0;

	for (auto vit = vertexList.Begin(); vit != vertexList.End(); ++vit)
	{
		source.Read(vit->position, sizeof(float) * 3);
		source.Read(vit->normal, sizeof(float) * 3);
		source.Read(vit->uv, sizeof(float) * 2);
		for (int j = 0; j < sizeInfo.UVVectorSize; ++j)
			source.Read(vit->uvEx[j], sizeof(float) * 4);
		vit->weightMethod = (PMX::VertexWeightMethod)source.ReadUByte();
		memset(&vit->boneInfo, 0, sizeof(vit->boneInfo));
		switch (vit->weightMethod)
		{
		case PMX::VertexWeightMethod::BDEF1:
			vit->boneInfo.BDEF.boneIndexes[0] = readAsU32(sizeInfo.BoneIndexSize, source);
			vit->boneInfo.BDEF.weights[0] = 1.0f;
			break;
		case PMX::VertexWeightMethod::BDEF2:
			for (i = 0; i < 2; i++)
				vit->boneInfo.BDEF.boneIndexes[i] = readAsU32(sizeInfo.BoneIndexSize, source);

			vit->boneInfo.BDEF.weights[0] = source.ReadFloat();
			vit->boneInfo.BDEF.weights[1] = 1.0f - vit->boneInfo.BDEF.weights[0];
			break;
		case PMX::VertexWeightMethod::QDEF:
			if (header.Version < 2.1f)
				throw Exception("QDEF not supported on PMX version lower than 2.1");
		case PMX::VertexWeightMethod::BDEF4:
			for (i = 0; i < 4; i++)
				vit->boneInfo.BDEF.boneIndexes[i] = readAsU32(sizeInfo.BoneIndexSize, source);
			source.Read(vit->boneInfo.BDEF.weights, sizeof(float) * 4);
			break;
		case PMX::VertexWeightMethod::SDEF:
			vit->boneInfo.SDEF.boneIndexes[0] = readAsU32(sizeInfo.BoneIndexSize, source);
			vit->boneInfo.SDEF.boneIndexes[1] = readAsU32(sizeInfo.BoneIndexSize, source);
			vit->boneInfo.SDEF.weightBias = source.ReadFloat();
			source.Read(vit->boneInfo.SDEF.C, sizeof(float) * 3);
			source.Read(vit->boneInfo.SDEF.R0, sizeof(float) * 3);
			source.Read(vit->boneInfo.SDEF.R1, sizeof(float) * 3);
			break;
		default:
			throw Exception("Invalid value for vertex weight method");
		}
		vit->edgeWeight = source.ReadFloat();

		vit->index = id++;
	}
}

void PMXModel::LoadIndexData(Urho3D::Vector<unsigned int>& indexList, Urho3D::Deserializer& source)
{
	indexList.Resize(source.ReadUInt());

	for (auto iit = indexList.Begin(); iit != indexList.End(); ++iit)
	{
		(*iit) = readAsU32(sizeInfo.VertexIndexSize, source);
	}
}

void PMXModel::LoadTextures(Urho3D::Vector<Urho3D::String>& textureList, Urho3D::Deserializer& source)
{
	textureList.Resize(source.ReadUInt());

	for (auto it = textureList.Begin(); it != textureList.End(); ++it)
	{
		*it = getString(source);
	}
}

void PMXModel::LoadMaterials(Urho3D::Vector<PMX::Material> &materialList, Urho3D::Deserializer& source)
{
	materialList.Resize(source.ReadUInt());

	for (auto it = materialList.Begin(); it != materialList.End(); ++it)
	{
		readName(it->name, source);

		source.Read(it->diffuse, sizeof(float) * 4);
		source.Read(it->specular, sizeof(float) * 3);
		it->specularCoefficient = source.ReadFloat();
		source.Read(it->ambient, sizeof(float) * 3);

		it->flags = source.ReadUByte();

		source.Read(it->edgeColor, sizeof(float) * 4);
		it->edgeSize = source.ReadFloat();

		it->baseTexture = readAsU32(sizeInfo.MaterialIndexSize, source);
		it->sphereTexture = readAsU32(sizeInfo.MaterialIndexSize, source);
		it->sphereMode = (PMX::MaterialSphereMode)source.ReadUByte();
		it->toonFlag = (PMX::MaterialToonMode)source.ReadUByte();

		switch (it->toonFlag) {
		case PMX::MaterialToonMode::DefaultTexture:
			it->toonTexture.default = source.ReadUByte();
			break;
		case PMX::MaterialToonMode::CustomTexture:
			it->toonTexture.custom = readAsU32(sizeInfo.TextureIndexSize, source);
			break;
		}

		it->freeField = getString(source);

		it->indexCount = source.ReadInt();
	}
}

void PMXModel::LoadBones(Urho3D::Vector<PMX::Bone>& boneList, Urho3D::Deserializer& source)
{
	boneList.Resize(source.ReadUInt());

	unsigned int id = 0;

	for (auto it = boneList.Begin(); it != boneList.End(); ++it)
	{
		readName(it->Name, source);

		source.Read(it->InitialPosition, sizeof(float) * 3);
		it->Parent = readAsU32(sizeInfo.BoneIndexSize, source);
		it->DeformationOrder = source.ReadInt();
		it->index = id++;

		it->Flags = source.ReadUShort();

		if (it->Flags & PMX::BoneFlags::Attached)
			it->Size.AttachTo = readAsU32(sizeInfo.BoneIndexSize, source);
		else source.Read(it->Size.Length, sizeof(float) * 3);

		if (it->Flags & (PMX::BoneFlags::RotationAttached | PMX::BoneFlags::TranslationAttached)) {
			it->Inherit.From = readAsU32(sizeInfo.BoneIndexSize, source);
			it->Inherit.Rate = source.ReadFloat();
		}
		else {
			it->Inherit.From = 0xFFFFFFFFU;
			it->Inherit.Rate = 1.0f;
		}

		if (it->Flags & PMX::BoneFlags::FixedAxis) {
			source.Read(it->AxisTranslation, sizeof(float) * 3);
		}

		if (it->Flags & PMX::BoneFlags::LocalAxis) {
			source.Read(it->LocalAxes.X, sizeof(float) * 3);
			source.Read(it->LocalAxes.Z, sizeof(float) * 3);
		}

		if (it->Flags & PMX::BoneFlags::OuterParentDeformation) {
			it->ExternalDeformationKey = source.ReadInt();
		}

		if (it->Flags & PMX::BoneFlags::IK) {
			it->IkData.targetIndex = readAsU32(sizeInfo.BoneIndexSize, source);
			it->IkData.loopCount = source.ReadInt();
			it->IkData.angleLimit = source.ReadFloat();

			it->IkData.links.Resize(source.ReadUInt());
			for (auto linkIt = it->IkData.links.Begin(); linkIt != it->IkData.links.End(); ++linkIt) {
				linkIt->boneIndex = readAsU32(sizeInfo.BoneIndexSize, source);
				linkIt->limitAngle = source.ReadBool();
				if (linkIt->limitAngle) {
					source.Read(linkIt->limits.lower, sizeof(float) * 3);
					source.Read(linkIt->limits.upper, sizeof(float) * 3);
				}
			}
		}
	}
}

void PMXModel::LoadMorphs(Urho3D::Vector<PMX::Morph> &morphList, Urho3D::Deserializer& source)
{
	morphList.Resize(source.ReadUInt());

	for (auto it = morphList.Begin(); it != morphList.End(); ++it)
	{
		readName(it->name, source);

		it->operation = source.ReadUByte();
		it->type = source.ReadUByte();
		it->data.Resize(source.ReadUInt());

		for (auto mdata = it->data.Begin(); mdata != it->data.End(); ++mdata)
		{
			switch (it->type)
			{
			case PMX::MorphType::Flip:
				if (header.Version < 2.1f)
					throw Exception("Flip morph not available for PMX version < 2.1");
			case PMX::MorphType::Group:
				mdata->group.index = readAsU32(sizeInfo.MorphIndexSize, source);
				mdata->group.rate = source.ReadFloat();
				break;
			case PMX::MorphType::Vertex:
				mdata->vertex.index = readAsU32(sizeInfo.VertexIndexSize, source);
				source.Read(mdata->vertex.offset, sizeof(float) * 3);
				break;
			case PMX::MorphType::Bone:
				mdata->bone.index = readAsU32(sizeInfo.BoneIndexSize, source);
				source.Read(mdata->bone.movement, sizeof(float) * 3);
				source.Read(mdata->bone.rotation, sizeof(float) * 4);
				break;
			case PMX::MorphType::UV:
			case PMX::MorphType::UV1:
			case PMX::MorphType::UV2:
			case PMX::MorphType::UV3:
			case PMX::MorphType::UV4:
				mdata->uv.index = readAsU32(sizeInfo.VertexIndexSize, source);
				source.Read(mdata->uv.offset, sizeof(float) * 4);
				break;
			case PMX::MorphType::Material:
				mdata->material.index = readAsU32(sizeInfo.MaterialIndexSize, source);
				mdata->material.method = (PMX::MaterialMorphMethod)source.ReadUByte();
				source.Read(mdata->material.diffuse, sizeof(float) * 4);
				source.Read(mdata->material.specular, sizeof(float) * 3);
				mdata->material.specularCoefficient = source.ReadFloat();
				source.Read(mdata->material.ambient, sizeof(float) * 3);
				source.Read(mdata->material.edgeColor, sizeof(float) * 4);
				mdata->material.edgeSize = source.ReadFloat();
				source.Read(mdata->material.baseCoefficient, sizeof(float) * 4);
				source.Read(mdata->material.sphereCoefficient, sizeof(float) * 4);
				source.Read(mdata->material.toonCoefficient, sizeof(float) * 4);
				break;
			case PMX::MorphType::Impulse:
				if (header.Version < 2.1f)
					throw Exception("Impulse morph not supported in PMX version < 2.1");

				mdata->impulse.index = readAsU32(sizeInfo.RigidBodyIndexSize, source);
				mdata->impulse.localFlag = source.ReadUByte();
				source.Read(mdata->impulse.velocity, sizeof(float) * 3);
				source.Read(mdata->impulse.rotationTorque, sizeof(float) * 3);
				break;
			default:
				throw Exception("Invalid morph type");
			}
		}
	}
}

void PMXModel::LoadFrames(Urho3D::Vector<PMX::Frame>& frameList, Urho3D::Deserializer& source)
{
	frameList.Resize(source.ReadUInt());

	for (auto it = frameList.Begin(); it != frameList.End(); ++it)
	{
		readName(it->name, source);
		it->type = source.ReadUByte();
		it->morphs.Resize(source.ReadUInt());

		for (auto morph = it->morphs.Begin(); morph != it->morphs.End(); ++morph)
		{
			morph->target = (PMX::FrameMorphTarget)source.ReadUByte();
			switch (morph->target) {
			case PMX::FrameMorphTarget::Bone:
				morph->id = readAsU32(sizeInfo.BoneIndexSize, source);
				break;
			case PMX::FrameMorphTarget::Morph:
				morph->id = readAsU32(sizeInfo.MorphIndexSize, source);
				break;
			}
		}
	}
}

void PMXModel::LoadRigidBodies(Urho3D::Vector<PMX::RigidBody>& rigidBodyList, Urho3D::Deserializer& source)
{
	rigidBodyList.Resize(source.ReadUInt());

	for (auto it = rigidBodyList.Begin(); it != rigidBodyList.End(); ++it)
	{
		readName(it->name, source);

		it->targetBone = readAsU32(sizeInfo.BoneIndexSize, source);

		it->group = source.ReadUByte();
		it->groupMask = source.ReadUShort();

		it->shape = (PMX::RigidBodyShape)source.ReadUByte();
		source.Read(it->size, sizeof(float) * 3);

		source.Read(it->position, sizeof(float) * 3);
		source.Read(it->rotation, sizeof(float) * 3);

		it->mass = source.ReadFloat();

		it->linearDamping = source.ReadFloat();
		it->angularDamping = source.ReadFloat();
		it->restitution = source.ReadFloat();
		it->friction = source.ReadFloat();

		it->mode = (PMX::RigidBodyMode)source.ReadUByte();
	}
}

void PMXModel::LoadJoints(Urho3D::Vector<PMX::Joint>& jointList, Urho3D::Deserializer& source)
{
	jointList.Resize(source.ReadUInt());

	for (auto it = jointList.Begin(); it != jointList.End(); ++it)
	{
		readName(it->name, source);

		it->type = (PMX::JointType)source.ReadUByte();

		if (header.Version < 2.1f && it->type != PMX::JointType::Spring6DoF)
			throw Exception("Invalid joint type for PMX version < 2.1");

		it->data.bodyA = readAsU32(sizeInfo.RigidBodyIndexSize, source);
		it->data.bodyB = readAsU32(sizeInfo.RigidBodyIndexSize, source);

		source.Read(it->data.position, sizeof(float) * 3);
		source.Read(it->data.rotation, sizeof(float) * 3);

		source.Read(it->data.lowerMovementRestrictions, sizeof(float) * 3);
		source.Read(it->data.upperMovementRestrictions, sizeof(float) * 3);
		source.Read(it->data.lowerRotationRestrictions, sizeof(float) * 3);
		source.Read(it->data.upperRotationRestrictions, sizeof(float) * 3);

		source.Read(it->data.springConstant, sizeof(float) * 6);
	}
}

void PMXModel::LoadSoftBodies(Urho3D::Vector<PMX::SoftBody>& softBodyList, Urho3D::Deserializer& source)
{
	softBodyList.Resize(source.ReadUInt());

	for (auto it = softBodyList.Begin(); it != softBodyList.End(); ++it)
	{
		readName(it->name, source);

		it->shape = (PMX::SoftBody::Shape)source.ReadUByte();
		it->material = readAsU32(sizeInfo.MaterialIndexSize, source);

		it->group = source.ReadUByte();
		it->groupFlags = source.ReadUShort();

		it->flags = (PMX::SoftBody::Flags)source.ReadUByte();

		it->blinkCreationDistance = source.ReadInt();
		it->clusterCount = source.ReadInt();

		it->mass = source.ReadFloat();
		it->collisionMargin = source.ReadFloat();

		it->model = (PMX::SoftBody::AeroModel)source.ReadUInt();

		source.Read(&it->config, sizeof(PMX::SoftBody::Config));
		source.Read(&it->cluster, sizeof(PMX::SoftBody::Cluster));
		source.Read(&it->iteration, sizeof(PMX::SoftBody::Iteration));
		source.Read(&it->materialInfo, sizeof(PMX::SoftBody::Material));

		it->anchors.Resize(source.ReadUInt());
		for (auto anchor = it->anchors.Begin(); anchor != it->anchors.End(); ++anchor)
		{
			anchor->rigidBodyIndex = readAsU32(sizeInfo.RigidBodyIndexSize, source);
			anchor->vertexIndex = readAsU32(sizeInfo.VertexIndexSize, source);
			anchor->nearMode = source.ReadUByte();
		}

		it->pins.Resize(source.ReadUInt());
		source.Read(&it->pins.At(0), sizeof(PMX::SoftBody::Pin) * it->pins.Size());
	}
}

template <class T>
T __getString(Urho3D::Deserializer& source)
{
	// Read length
	unsigned int len = source.ReadUInt();
	len /= sizeof(T::value_type);

	// Read the string itself
	T::pointer data = new T::value_type[len];
	source.Read(data, len * sizeof(T::value_type));
	T retval(data, len);
	delete[] data;
	return retval;
}

Urho3D::String PMXModel::getString(Urho3D::Deserializer& source) {
	static wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> conversor;

	switch (sizeInfo.Encoding) {
	case 0:
		return __getString<wstring>(source).c_str();
	case 1:
		return conversor.from_bytes(__getString<string>(source)).c_str();
	}

	return L"";
}

void PMXModel::readName(PMX::Name &name, Urho3D::Deserializer& source)
{
	name.japanese = getString(source);
	name.english = getString(source);
}

unsigned int PMXModel::readAsU32(unsigned char size, Urho3D::Deserializer& source)
{
	unsigned char u8val;
	unsigned short u16val;

	switch (size) {
	case 1:
		u8val = source.ReadUByte();
		if (u8val == 0xFF) return 0xFFFFFFFF;
		return u8val;
	case 2:
		u16val = source.ReadUShort();
		if (u16val == 0xFFFF) return 0xFFFFFFFF;
		return u16val;
	case 4:
		return source.ReadUInt();
	}

	return 0;
}

#if 0
bool PMX::Model::Update(float msec)
{
	if ((m_debugFlags & DebugFlags::DontUpdatePhysics) == 0) {
		for (auto &bone : m_prePhysicsBones) {
			bone->update();
		}

		for (auto &body : m_rigidBodies) {
			body->Update();
		}

		for (auto &bone : m_postPhysicsBones) {
			bone->update();
		}

		for (auto &bone : m_ikBones) {
			bone->performIK();
		}
	}

	return true;
}

void PMX::Model::Reset()
{
	for (auto &morph : morphs) {
		ApplyMorph(morph, 0.0f);
	}

	for (auto &bone : bones) {
		bone->clearIK();
		bone->resetTransform();
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
		applyImpulseMorph(morph, weight);
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

		v->MorphOffset = DirectX::XMVectorZero();
		for (auto &m : v->morphs) {
			v->MorphOffset = v->MorphOffset + DirectX::XMVectorSet(m->type->vertex.offset[0] * m->weight, m->type->vertex.offset[1] * m->weight, m->type->vertex.offset[2] * m->weight, 0.0f);
		}

		// Mark the material for update next frame
		for (auto &m : v->materials)
			m.first->dirty |= RenderMaterial::DirtyFlags::VertexBuffer;
	}
}

void PMX::Model::applyBoneMorph(Morph *morph, float weight)
{
	for (auto i : morph->data) {
		Bone *bone = bones[i.bone.index];
		bone->applyMorph(morph, weight);
	}
}

void PMX::Model::applyMaterialMorph(Morph *morph, float weight)
{
	for (auto i : morph->data) {
		// If target is -1, apply morph to all materials
		if (i.material.index == -1) {
			for (auto m : rendermaterials)
				applyMaterialMorph(&i, &m, weight);
			continue;
		}

		assert("Material morph index out of range" && i.material.index < rendermaterials.size());
		applyMaterialMorph(&i, &rendermaterials[i.material.index], weight);
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

void PMX::Model::applyImpulseMorph(Morph* morph, float weight)
{
	for (auto &Morph : morph->data) {
		assert("Impulse morph rigid body index out of range" && Morph.impulse.index < m_rigidBodies.size());
		auto Body = m_rigidBodies[Morph.impulse.index];
		assert("Impulse morph cannot be applied to a kinematic rigid body" && Body->isDynamic());
	}
}
#endif