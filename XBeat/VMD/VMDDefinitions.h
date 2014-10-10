#pragma once
#include <cstdint>

namespace VMD {

	struct BoneKeyFrame
	{
		char boneName[15];
		uint32_t frameCount;
		float translation[3];
		float rotation[4];
		int8_t interpolationData[64];
	};

	struct FaceKeyFrame
	{
		char name[15];
		uint32_t frameCount;
		float weight;
	};

	struct CameraKeyFrame
	{
		uint32_t frameCount;
		float distance;
		float position[3];
		float angles[3];
		int8_t interpolationData[24];
		uint32_t fovAngle;
		uint8_t noPerspective;
	};

}
