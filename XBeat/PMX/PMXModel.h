//===-- PMX/PMXModel.h - Defines the PMX model class ---------------*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===-------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares the PMX::Model class
///
//===-------------------------------------------------------------------------===//

#pragma once

#include "PMXDefinitions.h"

#include "PMXAnimatedModel.h"
#include "PMXSoftBody.h"

#include <Model.h>
#include <Vector.h>
#include <exception>

class PMXModel : public Urho3D::Model
{
	OBJECT(PMXModel);

	friend class PMXAnimatedModel;

	struct FileHeader {
		/// Stores the file identifier
		char Magic[4];
		/// Stores the type version
		float Version;
	} header;

 	struct FileSizeInfo {
		///! The size of this structure
		unsigned char Size;
		/// The encoding used for strings
		/// 0 means UTF-8, 1 means UTF-16
		unsigned char Encoding;
		/// The amount of UV components
		unsigned char UVVectorSize;
		/// The size of the vertex index, in bytes
		unsigned char VertexIndexSize;
		/// The size of the texture index, in bytes
		unsigned char TextureIndexSize;
		/// The size of the material index, in bytes
		unsigned char MaterialIndexSize;
		/// The size of the bone index, in bytes
		unsigned char BoneIndexSize;
		/// The size of the morph index, in bytes
		unsigned char MorphIndexSize;
		/// The size of the rigid body index, in bytes
		unsigned char RigidBodyIndexSize;
	} sizeInfo;

public:
	class Exception : public std::exception
	{
		Urho3D::String Message;

	public:
		Exception(const Urho3D::String &Message) throw() : Message(Message) {}

		virtual const char* what() const throw() {
			return Message.CString();
		}
	};

	/// Construct
	PMXModel(Urho3D::Context* context);
	/// Destruct
	virtual ~PMXModel();
	/// Register object factory.
	static void RegisterObject(Urho3D::Context* context);

	/// Returns this model description
	const PMX::ModelDescription& GetDescription() const { return description; }

	/// Load resource from stream. May be called from a worker thread. Return true if successful.
	virtual bool BeginLoad(Urho3D::Deserializer& source);
	/// Finish resource loading. Always called from the main thread. Return true if successful.
	virtual bool EndLoad();

	const Urho3D::Vector<PMX::Bone> GetBones() const { return boneList; }

private:
	Urho3D::Vector<PMX::Vertex> vertexList;
	Urho3D::Vector<unsigned int> indexList;
	Urho3D::Vector<Urho3D::String> textureList;
	Urho3D::Vector<PMX::Material> materialList;
	Urho3D::Vector<PMX::Bone> boneList;
	Urho3D::Vector<PMX::Morph> morphList;
	Urho3D::Vector<PMX::Frame> frameList;
	Urho3D::Vector<PMX::RigidBody> rigidBodyList;
	Urho3D::Vector<PMX::Joint> jointList;
	Urho3D::Vector<PMX::SoftBody> softBodyList;

	bool LoadHeader(FileHeader& header, Urho3D::Deserializer& source);
	void LoadSizeInfo(FileSizeInfo& sizeInfo, Urho3D::Deserializer& source);
	void LoadDescription(PMX::ModelDescription &description, Urho3D::Deserializer& source);
	void LoadVertexData(Urho3D::Vector<PMX::Vertex> &vertexList, Urho3D::Deserializer& source);
	void LoadIndexData(Urho3D::Vector<unsigned int> &indexList, Urho3D::Deserializer& source);
	void LoadTextures(Urho3D::Vector<Urho3D::String> &textureList, Urho3D::Deserializer& source);
	void LoadMaterials(Urho3D::Vector<PMX::Material> &materialList, Urho3D::Deserializer& source);
	void LoadBones(Urho3D::Vector<PMX::Bone> &boneList, Urho3D::Deserializer& source);
	void LoadMorphs(Urho3D::Vector<PMX::Morph> &morphList, Urho3D::Deserializer& source);
	void LoadFrames(Urho3D::Vector<PMX::Frame> &frameList, Urho3D::Deserializer& source);
	void LoadRigidBodies(Urho3D::Vector<PMX::RigidBody> &rigidBodyList, Urho3D::Deserializer& source);
	void LoadJoints(Urho3D::Vector<PMX::Joint> &jointList, Urho3D::Deserializer& source);
	void LoadSoftBodies(Urho3D::Vector<PMX::SoftBody> &softBodyList, Urho3D::Deserializer& source);

	Urho3D::String getString(Urho3D::Deserializer& source);
	void readName(PMX::Name &name, Urho3D::Deserializer& source);
	unsigned int readAsU32(unsigned char size, Urho3D::Deserializer& source);

	PMX::ModelDescription description;
};
