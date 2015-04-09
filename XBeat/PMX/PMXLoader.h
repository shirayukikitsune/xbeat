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
#include "PMXSoftBody.h"

#include <exception>
#include <fstream>
#include <istream>
#include <string>

#include <Ptr.h>

namespace Urho3D {
	class AnimatedModel;
}

namespace PMX {

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
		float InitialPosition[3];
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
		float AxisTranslation[3];
		struct {
			float X[3];
			float Z[3];
		} LocalAxes;
		int ExternalDeformationKey;
		IK IkData;

		uint32_t index;
	};

	struct RigidBody{
		Name name;
		uint32_t targetBone;
		uint8_t group;
		uint16_t groupMask;
		PMX::RigidBodyShape shape;
		float size[3];
		float position[3];
		float rotation[3];
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
			float position[3];
			float rotation[3];
			float lowerMovementRestrictions[3];
			float upperMovementRestrictions[3];
			float lowerRotationRestrictions[3];
			float upperRotationRestrictions[3];
			float springConstant[6];
		} data;
	};

	bool loadFromFile(Urho3D::AnimatedModel* Model, const std::wstring &Filename);
	bool loadFromStream(Urho3D::AnimatedModel* Model, std::istream &InStream);
	bool loadFromMemory(Urho3D::AnimatedModel* Model, const char *&Data);
	ModelDescription getDescription(const std::wstring &Filename);

private:
	FileHeader* loadHeader(const char *&data);
	FileSizeInfo* loadSizeInfo(const char *&data);
	void loadDescription(ModelDescription &desc, const char *&data);
	void loadVertexData(std::vector<Vertex> &VertexList, const char *&Data);
	void loadIndexData(std::vector<uint32_t> &IndexList, const char *&Data);
	void loadTextures(std::vector<std::wstring> &TextureList, const char *&Data);
	void loadMaterials(std::vector<Material> &MaterialList, const char *&Data);
	void loadBones(std::vector<Bone> &BoneList, const char *&Data);
	void loadMorphs(std::vector<Morph> &MorphList, const char *&Data);
	void loadFrames(std::vector<Frame> &FrameList, const char *&Data);
	void loadRigidBodies(std::vector<RigidBody> &RigidBodyList, const char *&Data);
	void loadJoints(std::vector<Joint> &JointList, const char *&Data);
	void loadSoftBodies(std::vector<SoftBody> &SoftBodyList, const char *&Data);

	std::wstring getString(const char *&Data);
	void readName(Name &name, const char *&Data);
	uint32_t readAsU32(uint8_t size, const char *&Data);
};

}
