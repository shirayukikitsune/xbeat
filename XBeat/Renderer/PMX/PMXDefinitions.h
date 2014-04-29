#pragma once

#include <cstdint>
#include <vector>
#include <list>
#include <D3D11.h>
#include <DirectXMath.h>
#include "../../Physics/Environment.h"

#ifdef PMX_TEST
namespace PMXTest {
	class BoneTest;
}
#endif

namespace Renderer {
class Texture;

namespace PMX {

#pragma region Basic Types
typedef btVector3 vec3f;
typedef btVector3 Position;
typedef btVector4 vec4f;

// Forward declarations here
class RenderMaterial;
class Bone;
struct Morph;

struct Color {
	float red;
	float green;
	float blue;

	void operator += (const Color &other) {
		red += other.red;
		green += other.green;
		blue += other.blue;
	}
	void operator *= (const Color &other) {
		red *= other.red;
		green *= other.green;
		blue *= other.blue;
	}
	void operator *= (float scalar) {
		red *= scalar;
		green *= scalar;
		blue *= scalar;
	}
};

struct Color4 : public Color {
	float alpha;

	void operator += (const Color4 &other) {
		red += other.red;
		green += other.green;
		blue += other.blue;
		alpha += other.alpha;
	}
	void operator *= (const Color4 &other) {
		red *= other.red;
		green *= other.green;
		blue *= other.blue;
		alpha *= other.alpha;
	}
	void operator *= (float scalar) {
		red *= scalar;
		green *= scalar;
		blue *= scalar;
		alpha *= scalar;
	}
};

struct Name {
	std::wstring japanese;
	std::wstring english;
};
#pragma endregion

#pragma region Enums
enum struct MaterialFlags : uint8_t {
	DoubleSide = 0x01,
	GroundShadow = 0x02,
	DrawSelfShadowMap = 0x04,
	DrawSelfShadow = 0x08,
	DrawEdge = 0x10
};

enum struct MaterialSphereMode : uint8_t {
	Disabled,
	Multiply,
	Add
};

enum struct VertexWeightMethod : uint8_t {
	BDEF1,
	BDEF2,
	BDEF4,
	SDEF,
	QDEF
};

enum struct BoneFlags : uint16_t {
	Attached = 0x0001,
	Rotatable = 0x0002,
	Movable = 0x0004,
	View = 0x0008,
	Manipulable = 0x0010,
	IK = 0x0020,
	LocalInheritance = 0x0080,
	InheritRotation = 0x0100,
	InheritTranslation = 0x0200,
	TranslateAxis = 0x0400,
	LocalAxis = 0x0800,
	AfterPhysicalDeformation = 0x1000,
	ExternalParentDeformation = 0x2000
};

enum struct DeformationOrigin {
	User,
	Morph,
	Inheritance,
	Internal
};

enum struct RigidBodyShape : uint8_t {
	Sphere,
	Box,
	Capsule
};

enum struct RigidBodyMode : uint8_t {
	Static,
	Dynamic,
	AlignedDynamic
};

enum struct JointType : uint8_t {
	Spring6DoF,
	SixDoF,
	PointToPoint,
	ConeTwist,
	Slider,
	Hinge
};

enum struct MaterialToonMode : uint8_t {
	CustomTexture,
	DefaultTexture,
};

enum struct MaterialMorphMethod : uint8_t {
	Multiplicative,
	Additive
};

enum struct FrameMorphTarget : uint8_t {
	Bone,
	Morph
};
#pragma endregion

ATTRIBUTE_ALIGNED16(struct) MaterialMorph{
	uint32_t index;
	MaterialMorphMethod method;
	Color4 diffuse;
	Color specular;
	float specularCoefficient;
	Color ambient;
	Color4 edgeColor;
	float edgeSize;
	Color4 baseCoefficient;
	Color4 sphereCoefficient;
	Color4 toonCoefficient;

	void operator+= (const MaterialMorph &other) {
		this->diffuse += other.diffuse;
		this->specular += other.specular;
		this->specularCoefficient += other.specularCoefficient;
		this->ambient += other.ambient;
		this->edgeColor += other.edgeColor;
		this->edgeSize += other.edgeSize;
		this->baseCoefficient += other.baseCoefficient;
		this->sphereCoefficient += other.sphereCoefficient;
		this->toonCoefficient += other.toonCoefficient;
	}
	void operator*= (const MaterialMorph &other) {
		this->diffuse *= other.diffuse;
		this->specular *= other.specular;
		this->specularCoefficient *= other.specularCoefficient;
		this->ambient *= other.ambient;
		this->edgeColor *= other.edgeColor;
		this->edgeSize *= other.edgeSize;
		this->baseCoefficient *= other.baseCoefficient;
		this->sphereCoefficient *= other.sphereCoefficient;
		this->toonCoefficient *= other.toonCoefficient;
	}

	BT_DECLARE_ALIGNED_ALLOCATOR();
};

union MorphType {
	struct {
		uint32_t index;
		btScalar offset[3];
	} vertex;
	struct {
		uint32_t index;
		btScalar offset[4];
	} uv;
	struct {
		uint32_t index;
		btScalar movement[3];
		btScalar rotation[4];
	} bone;
	MaterialMorph material;
	struct {
		uint32_t index;
		float rate;
	} group;
	struct {
		uint32_t index; // Rigid Body index
		uint8_t localFlag;
		btScalar velocity[3];
		btScalar rotationTorque[3]; // If all 0, stop all rotation
	} impulse;

	// Just for organization, so we must use MorphType::Group for example, it is not used in the union itself
	enum MorphTypeId {
		Group,
		Vertex,
		Bone,
		UV,
		UV1,
		UV2,
		UV3,
		UV4,
		Material,
		Flip,
		Impulse
	};
};

ATTRIBUTE_ALIGNED16(struct) Vertex{
	Position position;
	Position normal;
	float uv[2];
	vec4f uvEx[4];
	VertexWeightMethod weightMethod;
	union {
		struct {
			uint32_t boneIndexes[4];
			float weights[4];
		} BDEF;
		struct {
			uint32_t boneIndexes[2];
			float weightBias;
			btScalar C[3];
			btScalar R0[4];
			btScalar R1[4];
		} SDEF;
	} boneInfo;
	float edgeWeight;

	Bone *bones[4];
	//RenderMaterial *material;
	std::list<std::pair<RenderMaterial*, UINT>> materials;
	Position boneOffset[4], morphOffset;
	btQuaternion boneRotation[4];
	struct MorphData {
		Morph *morph;
		MorphType *type;
		float weight;
	};
	std::list<MorphData*> morphs;

	inline Position GetFinalPosition() {
		/*Position p = morphOffset + boneOffset[0] + boneOffset[1] + boneOffset[2] + boneOffset[3];
		btTransform t;
		t.setOrigin(this->position);
		t.setRotation(boneRotation[0] * boneRotation[1] * boneRotation[2] * boneRotation[3]);
		return t(p);*/
		return position + morphOffset + boneOffset[0] + boneOffset[1] + boneOffset[2] + boneOffset[3];
	}
	inline Position GetNormal() {
		btTransform t;
		t.setIdentity();
		t.setRotation(boneRotation[0] * boneRotation[1] * boneRotation[2] * boneRotation[3]);
		return t(normal).normalized();
	}

	BT_DECLARE_ALIGNED_ALLOCATOR();
};

ATTRIBUTE_ALIGNED16(struct) Material{
	Name name;
	Color4 diffuse;
	Color specular;
	float specularCoefficient;
	Color ambient;
	uint8_t flags;
	Color4 edgeColor;
	float edgeSize;
	uint32_t baseTexture;
	uint32_t sphereTexture;
	MaterialSphereMode sphereMode;
	MaterialToonMode toonFlag;
	union {
		uint32_t custom;
		uint8_t default;
	} toonTexture;
	std::wstring freeField;
	int indexCount;

	BT_DECLARE_ALIGNED_ALLOCATOR();
};

ATTRIBUTE_ALIGNED16(struct) IK{
	struct Node {
		uint32_t bone;
		bool limitAngle;
		struct {
			Position lower;
			Position upper;
		} limits;
	};

	uint32_t begin;
	int loopCount;
	float angleLimit;
	std::vector<Node> links;

	BT_DECLARE_ALIGNED_ALLOCATOR();
};

ATTRIBUTE_ALIGNED16(struct) Morph{
	Morph() {
		appliedWeight = 0.0f;
	};

	Name name;
	uint8_t operation;
	uint8_t type;
	std::vector<MorphType> data;

	float appliedWeight;

	BT_DECLARE_ALIGNED_ALLOCATOR();
};

ATTRIBUTE_ALIGNED16(struct) FrameMorphs{
	FrameMorphTarget target;
	uint32_t id;

	BT_DECLARE_ALIGNED_ALLOCATOR();
};

ATTRIBUTE_ALIGNED16(struct) Frame{
	Name name;
	uint8_t type;
	std::vector<FrameMorphs> morphs;

	BT_DECLARE_ALIGNED_ALLOCATOR();
};

ATTRIBUTE_ALIGNED16(struct) Joint{
	Name name;
	JointType type;
	struct {
		uint32_t bodyA, bodyB;
		vec3f position;
		vec3f rotation;
		vec3f lowerMovementRestrictions;
		vec3f upperMovementRestrictions;
		vec3f lowerRotationRestrictions;
		vec3f upperRotationRestrictions;
		vec3f movementSpringConstant;
		vec3f rotationSpringConstant;
	} data;

	BT_DECLARE_ALIGNED_ALLOCATOR();
};
}
}

