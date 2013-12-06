#pragma once
#include <vector>
#include <istream>
#include <cstdint>

#include "PMXDefinitions.h"
#include "../../Physics/Environment.h"
#include "BulletSoftBody/btSoftBody.h"

namespace Renderer {
namespace PMX {

class Model;

class SoftBody
{
public:
	SoftBody(void);
	~SoftBody(void);

	struct Shape {
		enum Shape_e : uint8_t {
			TriangleMesh,
			Rope
		};
	};
	struct Flags {
		enum Flags_e : uint8_t {
			BLinkCreate = 0x01,
			CreateCluster = 0x02,
			LinkCrossing = 0x04
		};
	};
	struct AeroModel {
		enum AeroModel_e : uint32_t {
			V_Point,
			V_TwoSided,
			V_SingleSided,
			F_TwoSided,
			F_SingleSided
		};
	};

	Name name;
	Shape::Shape_e shape;
	uint32_t material;
	uint8_t group;
	uint16_t groupFlags;
	Flags::Flags_e flags;
	int blinkCreationDistance;
	int clusterCount;
	float mass;
	float collisionMargin;
	AeroModel::AeroModel_e model;
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
		uint32_t rigidBodyIndex;
		uint32_t vertexIndex;
		uint8_t  nearMode;
	};
	std::vector<AnchorRigidBody> anchors;

	struct Pin {
		uint32_t vertexIndex;
	};
	std::vector<Pin> pins;

	void Create(std::shared_ptr<Physics::Environment> physics, Model* model);
};

}
}
