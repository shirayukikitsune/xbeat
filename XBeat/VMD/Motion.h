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

#include "../PMX/PMXModel.h"
#include "../Renderer/Camera.h"

#include "VMDDefinitions.h"

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

		/// \brief Resets the animation state
		void reset();

		/// \brief Loads a motion from a file
		///
		/// \param [in] FileName The path of the motion file to be loaded
		/// \returns Whether the loading was successful or not
		bool loadFromFile(const std::wstring &FileName);

		/// \brief Advances the frame of the motion
		///
		/// \param [in] Frames The amount of frames to advance the motion
		/// \returns true if the animation is finished, false otherwise
		bool advanceFrame(float Frames);

		/// \brief Attaches a Renderer::Camera to the motion
		///
		/// \param [in] Camera The camera to be attached
		void attachCamera(std::shared_ptr<Renderer::Camera> Camera);
		/// \brief Attaches a Renderer::Model to the motion
		///
		/// \param [in] Model The model to be attached
		void attachModel(std::shared_ptr<PMX::Model> Model);

		/// \brief Returns the motion finished state
		bool isFinished() { return Finished; }

	private:
		/// \brief The current frame of the motion
		float CurrentFrame;
		/// \brief The motion frame count
		float MaxFrame;

		/// \brief Defines whether this motion has finished or not
		bool Finished;

		/// \brief The key frames of bone animations
		std::map<std::wstring, std::vector<BoneKeyFrame>> BoneKeyFrames;
		/// \brief The key frames of morphs animations
		std::map<std::wstring, std::vector<MorphKeyFrame>> MorphKeyFrames;
		/// \brief The key frames of camera animations
		std::vector<CameraKeyFrame> CameraKeyFrames;

		/// \brief The attached cameras
		std::vector<std::shared_ptr<Renderer::Camera>> AttachedCameras;
		/// \brief The attached models
		std::vector<std::shared_ptr<PMX::Model>> AttachedModels;

		/// \brief Apply motion parameters to all attached cameras
		void setCameraParameters(float FieldOfView, float Distance, btVector3 &Position, btQuaternion &Rotation);

		/// \brief Apply bone deformation parameters to all attached models
		void setBoneParameters(std::wstring BoneName, btVector3 &Translation, btQuaternion &Rotation);
		
		/// \brief Apply morph parameters to all attached models
		void setMorphParameters(std::wstring MorphName, float MorphWeight);

		/// \name Functions extracted from MMDAgent, http://www.mmdagent.jp/
		/// @{

		/// \brief Sets camera parameters according to the motion at the specified frame
		///
		/// \param [in] Frame The frame of the animation
		void updateCamera(float Frame);

		/// \brief Parses the camera interpolation data from the VMD file
		void parseCameraInterpolationData(CameraKeyFrame &Frame, int8_t *InterpolationData);

		/// \brief Sets bones parameters according to the motion at the specified frame
		void updateBones(float Frame);

		/// \brief Parses the bone interpolation data from the VMD file
		void parseBoneInterpolationData(BoneKeyFrame &Frame, int8_t *InterpolationData);

		/// \brief Sets morphs parameters according to the motion at the specified frame
		void updateMorphs(float Frame);

		/// \brief Generates the interpolation data table
		void generateInterpolationTable(std::vector<float> &Table, float X1, float X2, float Y1, float Y2);

		/// \brief Cubic Bézier curve interpolation function
		///
		/// \param [in] T The interpolation value, in [0.0; 1.0] range
		/// \param [in] P1 The first point ordinate
		/// \param [in] P2 The second point ordinate
		float InterpolationFunction(float T, float P1, float P2);

		/// \brief The derivative of the cubic Bézier curve
		/// \sa float VMD::Motion::InterpolationFunction(float T, float P1, float P2)
		float InterpolationFunctionDerivative(float T, float P1, float P2);

		float doLinearInterpolation(float Ratio, float V1, float V2) {
			return V1 * (1.0f - Ratio) + V2 * Ratio;
		}

		/// @}

		enum {
			InterpolationTableSize = 64
		};
	};

}

