#pragma once

#include <string>
#include <fstream>
#include <istream>
#include <exception>

#include "PMXDefinitions.h"

namespace Renderer {
namespace PMX {

class Model;

class Loader {
	struct Header {
		char abMagic[4];
		float fVersion;
	} *m_header;

	struct SizeInfo {
		uint8_t cbSize;
		uint8_t cbEncoding;
		uint8_t cbUVVectorSize;
		uint8_t cbVertexIndexSize;
		uint8_t cbTextureIndexSize;
		uint8_t cbMaterialIndexSize;
		uint8_t cbBoneIndexSize;
		uint8_t cbMorphIndexSize;
		uint8_t cbRigidBodyIndexSize;
	} *m_sizeInfo;

public:
	class Exception : public std::exception
	{
		std::string msg;

	public:
		Exception(const std::string &_msg) throw() : msg(_msg) {}

		virtual const char* what() const throw() {
			return msg.c_str();
		}
	};

	PMX_ALIGN struct RigidBody {
		Name name;
		uint32_t targetBone;
		uint8_t group;
		uint16_t groupMask;
		PMX::RigidBodyShape::Id shape;
		vec3f size;
		vec3f position;
		vec3f rotation;
		float mass;
		float linearDamping;
		float angularDamping;
		float restitution;
		float friction;
		PMX::RigidBodyMode::Id mode;

		PMX_ALIGNMENT_OPERATORS
	};

	bool FromFile(Model* model, const std::wstring &filename);
	bool FromStream(Model* model, std::istream &in);
	bool FromMemory(Model* model, const char *&data);

private:
	Header* loadHeader(const char *&data);
	SizeInfo* loadSizeInfo(const char *&data);
	void loadDescription(Model* model, const char *&data);
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
}
