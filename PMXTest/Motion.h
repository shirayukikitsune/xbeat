#pragma once

#include <string>
#include <vector>

namespace VMD
{

	struct KeyFrame
	{
		char boneName[15];
		uint32_t frameCount;
		float boneX;
		float boneY;
		float boneZ;
		float rotationX;
		float rotationY;
		float rotationZ;
		float rotationW;
		char lerpData[64];
	};

	struct FaceKeyFrame
	{
		char transformation[15];
		uint32_t frameCount;
		float weight;
	};

class Motion
{
public:
	Motion();
	~Motion();

	bool Load(const std::wstring &file);

	inline uint32_t GetKeyFrameCount() const { return keyFrames.size(); }
	inline uint32_t GetFaceKeyFrameCount() const { return faceKeyFrames.size(); }

	const KeyFrame& GetKeyFrame(uint32_t index) const;
	const FaceKeyFrame& GetFaceKeyFrame(uint32_t index) const;

private:
	std::vector<KeyFrame> keyFrames;
	std::vector<FaceKeyFrame> faceKeyFrames;
};

}
