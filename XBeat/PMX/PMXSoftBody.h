#pragma once
#include "PMXDefinitions.h"

namespace PMX {

struct SoftBody
{
	enum struct Shape : unsigned char {
		TriangleMesh,
		Rope
	};
	enum struct Flags : unsigned char {
		BLinkCreate = 0x01,
		CreateCluster = 0x02,
		LinkCrossing = 0x04
	};
	enum struct AeroModel : unsigned int {
		V_Point,
		V_TwoSided,
		V_SingleSided,
		F_TwoSided,
		F_SingleSided
	};

	Name name;
	Shape shape;
	unsigned int material;
	unsigned char group;
	unsigned short groupFlags;
	Flags flags;
	int blinkCreationDistance;
	int clusterCount;
	float mass;
	float collisionMargin;
	AeroModel model;
	struct Config {
		float VCF;
		float DP;
		float DG;
		float LF;
		float PR;
		float VC;
		float DF;
		float MT;
		float CHR;
		float KHR;
		float SHR;
		float AHR;
	} config;
	struct Cluster {
		float SRHR;
		float SKHR;
		float SSHR;
		float SR_SPLT;
		float SK_SPLT;
		float SS_SPLT;
	} cluster;
	struct Iteration {
		float V;
		float P;
		float D;
		float C;
	} iteration;

	struct Material {
		float L;
		float A;
		float V;
	} materialInfo;

	struct AnchorRigidBody {
		unsigned int rigidBodyIndex;
		unsigned int vertexIndex;
		unsigned char nearMode;
	};
	Urho3D::Vector<AnchorRigidBody> anchors;

	struct Pin {
		unsigned int vertexIndex;
	};
	Urho3D::Vector<Pin> pins;
};

}
