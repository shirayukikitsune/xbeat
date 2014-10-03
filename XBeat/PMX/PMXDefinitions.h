#pragma once

#include <cstdint>
#include <vector>
#include <list>
#include <D3D11.h>
#include <DirectXMath.h>
#include "../Physics/Environment.h"

#ifdef PMX_TEST
namespace PMXTest {
	class BoneTest;
}
#endif

namespace Renderer { class Texture; }

namespace PMX {

#pragma region Basic Types
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

struct ModelDescription {
	Name name;
	Name comment;
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
	QDEF,
	Count
};

enum struct BoneFlags : uint16_t {
	Attached = 0x0001,
	Rotatable = 0x0002,
	Movable = 0x0004,
	View = 0x0008,
	Manipulable = 0x0010,
	IK = 0x0020,
	LocallyAttached = 0x0080,
	RotationAttached = 0x0100,
	TranslationAttached = 0x0200,
	FixedAxis = 0x0400,
	LocalAxis = 0x0800,
	PostPhysicsDeformation = 0x1000,
	OuterParentDeformation = 0x2000
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

struct MaterialMorph{
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
};

union MorphType {
	struct {
		uint32_t index;
		float offset[3];
	} vertex;
	struct {
		uint32_t index;
		float offset[4];
	} uv;
	struct {
		uint32_t index;
		float movement[3];
		float rotation[4];
	} bone;
	MaterialMorph material;
	struct {
		uint32_t index;
		float rate;
	} group;
	struct {
		uint32_t index; // Rigid Body index
		uint8_t localFlag;
		float velocity[3];
		float rotationTorque[3]; // If all 0, stop all rotation
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

struct Vertex {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	float uv[2];
	DirectX::XMFLOAT4 uvEx[4];
	VertexWeightMethod weightMethod;
	union {
		struct {
			uint32_t boneIndexes[4];
			float weights[4];
		} BDEF;
		struct {
			uint32_t boneIndexes[2];
			float weightBias;
			float C[3];
			float R0[3];
			float R1[3];
		} SDEF;
	} boneInfo;
	float edgeWeight;

	Bone *bones[4];
	std::list<std::pair<RenderMaterial*, UINT>> materials;
	struct MorphData {
		Morph *morph;
		MorphType *type;
		float weight;
	};
	std::list<MorphData*> morphs;

	uint32_t index;
};

struct Material{
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
};

struct IK{
	struct Node {
		uint32_t bone;
		bool limitAngle;
		struct {
			DirectX::XMVECTOR lower;
			DirectX::XMVECTOR upper;
		} limits;
	};

	uint32_t begin;
	int loopCount;
	float angleLimit;
	std::vector<Node> links;
};

struct Morph{
	Morph() {
		appliedWeight = 0.0f;
	};

	Name name;
	uint8_t operation;
	uint8_t type;
	std::vector<MorphType> data;

	float appliedWeight;
};

struct FrameMorphs{
	FrameMorphTarget target;
	uint32_t id;
};

struct Frame{
	Name name;
	uint8_t type;
	std::vector<FrameMorphs> morphs;
};

}

