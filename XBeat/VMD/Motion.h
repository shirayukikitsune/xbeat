//===-- VMD/Motion.h - Declares the VMD animation class ------------*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===-----------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares the VMD::Motion class
///
//===-----------------------------------------------------------------------===//

#pragma once

#include "VMDDefinitions.h"

#include "../PMX/PMXModel.h"
#include "../Renderer/Camera.h"

#include <queue>
#include <string>
#include <vector>

namespace VMD {

	/// \brief This is used to perform an animation of a character and/or camera
	///
	/// \remarks A single character may have different motions (for example, one motion is related to walking and another related to wave hands) in effect at the same time.
	class Motion
	{
	public:
		Motion();
		~Motion();

		/// \brief Loads a motion from a file
		///
		/// \param [in] FileName The path of the motion file to be loaded
		/// \returns Whether the loading was successful or not
		bool loadFromFile(const std::wstring &FileName);

		/// \brief Advances the time of the motion
		///
		/// \param [in] Time The amount of time, in milliseconds, to advance the motion
		void advanceTime(float Time);

		/// \brief Attaches a Renderer::Camera to the motion
		///
		/// \param [in] Camera The camera to be attached
		void attachCamera(std::shared_ptr<Renderer::Camera> Camera);
		/// \brief Attaches a Renderer::Model to the motion
		///
		/// \param [in] Model The model to be attached
		void attachModel(std::shared_ptr<Renderer::Model> Model);

	private:
		/// \brief The current time of the motion
		float CurrentTime;

		/// \brief The key frames of bone animations
		std::queue<BoneKeyFrame> BoneKeyFrames;
		/// \brief The key frames of morphs animations
		std::queue<MorphKeyFrame> MorphKeyFrames;
		/// \brief The key frames of camera animations
		std::queue<CameraKeyFrame> CameraKeyFrames;

		/// \brief The attached cameras
		std::vector<std::shared_ptr<Renderer::Camera>> AttachedCameras;
		/// \brief The attached models
		std::vector<std::shared_ptr<PMX::Model>> AttachedModels;
	};

}

