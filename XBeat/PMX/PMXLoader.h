//===-- PMX/PMXLoader.h - Declares the PMX loading class ------------*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares the PMX::Loader class
///
//===--------------------------------------------------------------------------===//

#pragma once

#include "PMXDefinitions.h"

#include <exception>
#include <fstream>
#include <istream>
#include <string>

namespace PMX {

class Model;

class Loader {
	struct FileHeader {
		/**
		 * \brief Stores the file identifier
		 */
		char Magic[4];
		/**
		 * \brief Stores the type version
		 */
		float Version;
	} *Header;

	struct FileSizeInfo {
		/**
		 * \brief The size of this structure
		 */
		uint8_t Size;
		/**
		 * \brief The encoding used for strings
		 */
		uint8_t Encoding;
		/**
		 * \brief The amount of UV components
		 */
		uint8_t UVVectorSize;
		/**
		 * \brief The size of the vertex index, in bytes
		 */
		uint8_t VertexIndexSize;
		/**
		 * \brief The size of the texture index, in bytes
		 */
		uint8_t TextureIndexSize;
		/**
		 * \brief The size of the material index, in bytes
		 */
		uint8_t MaterialIndexSize;
		/**
		 * \brief The size of the bone index, in bytes
		 */
		uint8_t BoneIndexSize;
		/**
		 * \brief The size of the morph index, in bytes
		 */
		uint8_t MorphIndexSize;
		/**
		 * \brief The size of the rigid body index, in bytes
		 */
		uint8_t RigidBodyIndexSize;
	} *SizeInfo;

public:
	class Exception : public std::exception
	{
		std::string Message;

	public:
		Exception(const std::string &Message) throw() : Message(Message) {}

		virtual const char* what() const throw() {
			return Message.c_str();
		}
	};

	struct Bone {
		Name Name;
		DirectX::XMFLOAT3 InitialPosition;
		uint32_t Parent;
		int DeformationOrder;
		uint16_t Flags;
		union {
			float Length[3];
			uint32_t AttachTo;
		} Size;
		struct {
			uint32_t From;
			float Rate;
		} Inherit;
		DirectX::XMFLOAT3 AxisTranslation;
		struct {
			DirectX::XMFLOAT3 X;
			DirectX::XMFLOAT3 Z;
		} LocalAxes;
		int ExternalDeformationKey;
		IK *IkData;
	};

	struct RigidBody{
		Name name;
		uint32_t targetBone;
		uint8_t group;
		uint16_t groupMask;
		PMX::RigidBodyShape shape;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 rotation;
		float mass;
		float linearDamping;
		float angularDamping;
		float restitution;
		float friction;
		PMX::RigidBodyMode mode;
	};

	struct Joint{
		Name name;
		JointType type;
		struct {
			uint32_t bodyA, bodyB;
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT3 rotation;
			DirectX::XMFLOAT3 lowerMovementRestrictions;
			DirectX::XMFLOAT3 upperMovementRestrictions;
			DirectX::XMFLOAT3 lowerRotationRestrictions;
			DirectX::XMFLOAT3 upperRotationRestrictions;
			float springConstant[6];
		} data;
	};

	bool loadFromFile(Model* model, const std::wstring &filename);
	bool loadFromStream(Model* model, std::istream &in);
	bool loadFromMemory(Model* model, const char *&data);
	ModelDescription getDescription(const std::wstring &filename);

	std::vector<Loader::Bone> Bones;
	std::vector<Loader::RigidBody> RigidBodies;
	std::vector<Loader::Joint> Joints;

private:
	FileHeader* loadHeader(const char *&data);
	FileSizeInfo* loadSizeInfo(const char *&data);
	void loadDescription(ModelDescription &desc, const char *&data);
	void loadVertexData(Model* model, const char *&data);
	void loadIndexData(Model* model, const char *&data);
	void loadTextures(Model* model, const char *&data);
	void loadMaterials(Model* model, const char *&data);
	void loadBones(Model* model, const char *&data);
	void loadMorphs(Model* model, const char *&data);
	void loadFrames(Model* model, const char *&data);
	void loadRigidBodies(Model* model, const char *&data);
	void loadJoints(Model* model, const char *&data);
	void loadSoftBodies(Model* model, const char *&data);

	std::wstring getString(const char *&data);
	void readName(Name &name, const char *&data);
	uint32_t readAsU32(uint8_t size, const char *&data);
};

}
