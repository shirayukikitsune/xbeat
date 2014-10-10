#pragma once

#include "VMDDefinitions.h"
#include <string>
#include <vector>
#include "../Renderer/Camera.h"
#include "../PMX/PMXModel.h"

namespace VMD {

	class Motion
	{
	public:
		Motion();
		~Motion();

		bool Load(const std::wstring &file);
		void AdvanceTime(float msec);

		void AttachCamera(std::shared_ptr<Renderer::Camera> camera);
		void AttachModel(std::shared_ptr<Renderer::Model> camera);

		const BoneKeyFrame& GetKeyFrame(uint32_t index) const;
		const FaceKeyFrame& GetFaceKeyFrame(uint32_t index) const;

	private:
		float currentTime;

		std::vector<BoneKeyFrame> boneKeyFrames;
		std::vector<FaceKeyFrame> faceKeyFrames;
		std::vector<CameraKeyFrame> cameraKeyFrames;

		std::vector<std::shared_ptr<Renderer::Camera>> attachedCameras;
		std::vector<std::shared_ptr<PMX::Model>> attachedModels;
	};

}

