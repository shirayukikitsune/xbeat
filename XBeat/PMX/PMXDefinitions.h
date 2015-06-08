//===-- PMX/PMXLoader.h - Contains various definitions about the PMX --*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------------===//
///
/// \file
/// \brief This file contains several definitions used in the PMX::Loader class
///
//===----------------------------------------------------------------------------===//

#pragma once

#include <Str.h>
#include <Vector.h>

namespace PMX {

#pragma region Basic Types
// Forward declarations here
struct Morph;

struct Name {
	Urho3D::String japanese;
	Urho3D::String english;
};

struct ModelDescription {
	Name name;
	Name comment;
};
#pragma endregion

#pragma region Enums
enum struct MaterialFlags : unsigned char {
	DoubleSide = 0x01,
	GroundShadow = 0x02,
	DrawSelfShadowMap = 0x04,
	DrawSelfShadow = 0x08,
	DrawEdge = 0x10
};

enum struct MaterialSphereMode : unsigned char {
	Disabled,
	Multiply,
	Add
};

enum struct VertexWeightMethod : unsigned char {
	BDEF1,
	BDEF2,
	BDEF4,
	SDEF,
	QDEF,
	Count
};

enum struct BoneType {
	Root,
	Regular,
	IK,
};

namespace BoneFlags {
	static const unsigned short Attached = 0x0001;
	static const unsigned short Rotatable = 0x0002;
	static const unsigned short Movable = 0x0004;
	static const unsigned short Visible = 0x0008;
	static const unsigned short Manipulable = 0x0010;
	static const unsigned short IK = 0x0020;
	static const unsigned short LocallyAttached = 0x0020;
	static const unsigned short RotationAttached = 0x0100;
	static const unsigned short TranslationAttached = 0x0200;
	static const unsigned short FixedAxis = 0x0400;
	static const unsigned short LocalAxis = 0x0800;
	static const unsigned short PostPhysicsDeformation = 0x1000;
	static const unsigned short OuterParentDeformation = 0x2000;
};

enum struct DeformationOrigin {
	User,
	Morph,
	Motion,
	Inheritance,
	Internal
};

enum struct RigidBodyShape : unsigned char {
	Sphere,
	Box,
	Capsule
};

enum struct RigidBodyMode : unsigned char {
	Static,
	Dynamic,
	AlignedDynamic
};

enum struct JointType : unsigned char {
	Spring6DoF,
	SixDoF,
	PointToPoint,
	ConeTwist,
	Slider,
	Hinge
};

enum struct MaterialToonMode : unsigned char {
	CustomTexture,
	DefaultTexture,
};

enum struct MaterialMorphMethod : unsigned char {
	Multiplicative,
	Additive
};

enum struct FrameMorphTarget : unsigned char {
	Bone,
	Morph
};
#pragma endregion

struct MaterialMorph {
	unsigned int index;
	MaterialMorphMethod method;
	float diffuse[4];
	float specular[3];
	float specularCoefficient;
	float ambient[3];
	float edgeColor[4];
	float edgeSize;
	float baseCoefficient[4];
	float sphereCoefficient[4];
	float toonCoefficient[4];
};

union MorphType {
	struct {
		unsigned int index;
		float offset[3];
	} vertex;
	struct {
		unsigned int index;
		float offset[4];
	} uv;
	struct {
		unsigned int index;
		float movement[3];
		float rotation[4];
	} bone;
	MaterialMorph material;
	struct {
		unsigned int index;
		float rate;
	} group;
	struct {
		unsigned int index; // Rigid Body index
		unsigned char localFlag;
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
	float position[3];
	float normal[3];
	float uv[2];
	float uvEx[4][4];
	VertexWeightMethod weightMethod;
	union {
		struct {
			unsigned int boneIndexes[4];
			float weights[4];
		} BDEF;
		struct {
			unsigned int boneIndexes[2];
			float weightBias;
			float C[3];
			float R0[3];
			float R1[3];
		} SDEF;
	} boneInfo;
	float edgeWeight;

	unsigned int index;
};

struct Material{
	Name name;
	float diffuse[4];
	float specular[3];
	float specularCoefficient;
	float ambient[3];
	unsigned char flags;
	float edgeColor[4];
	float edgeSize;
	unsigned int baseTexture;
	unsigned int sphereTexture;
	MaterialSphereMode sphereMode;
	MaterialToonMode toonFlag;
	union {
		unsigned int custom;
		unsigned char default;
	} toonTexture;
	Urho3D::String freeField;
	int indexCount;
};

struct IK {
	struct Node {
		unsigned int boneIndex;
		bool limitAngle;
		struct {
			float lower[3];
			float upper[3];
		} limits;
	};

	unsigned int targetIndex;
	int loopCount;
	float angleLimit;
	Urho3D::Vector<Node> links;
};

struct Morph{
	Morph() {
		appliedWeight = 0.0f;
	};

	Name name;
	unsigned char operation;
	unsigned char type;
	Urho3D::Vector<MorphType> data;

	float appliedWeight;
};

struct FrameMorphs{
	FrameMorphTarget target;
	unsigned int id;
};

struct Frame{
	Name name;
	unsigned char type;
	Urho3D::Vector<FrameMorphs> morphs;
};


struct Bone {
	Name Name;
	float InitialPosition[3];
	unsigned int Parent;
	int DeformationOrder;
	unsigned short Flags;
	union {
		float Length[3];
		unsigned int AttachTo;
	} Size;
	struct {
		unsigned int From;
		float Rate;
	} Inherit;
	float AxisTranslation[3];
	struct {
		float X[3];
		float Z[3];
	} LocalAxes;
	int ExternalDeformationKey;
	IK IkData;

	unsigned int index;
};

struct RigidBody{
	Name name;
	unsigned int targetBone;
	unsigned char group;
	unsigned short groupMask;
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
		unsigned int bodyA, bodyB;
		float position[3];
		float rotation[3];
		float lowerMovementRestrictions[3];
		float upperMovementRestrictions[3];
		float lowerRotationRestrictions[3];
		float upperRotationRestrictions[3];
		float springConstant[6];
	} data;
};
}

