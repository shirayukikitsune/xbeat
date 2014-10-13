//===-- VMD/VMDDefinitions.h - Declares support classes for VMD animations --*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares support classes, enums and structs related to the VMD
/// animation
///
//===--------------------------------------------------------------------------------===//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <LinearMath/btVector3.h>

namespace VMD {

	/// \brief Stores information about the bone animation
	struct BoneKeyFrame
	{
		/// \brief The name of the bone to be animated
		std::wstring BoneName;
		/// \brief The amount of frames until the next keyframe
		uint32_t FrameCount;
		/// \brief The bone translation component
		float Translation[3];
		/// \brief The bone rotation component, a quaternion
		float Rotation[4];
		/// \brief Bone interpolation data
		int8_t InterpolationData[64];
	};

	/// \brief Stores information about morph animations
	struct MorphKeyFrame
	{
		/// \brief The name of the morph to be applied, Shift-JIS encoded
		std::wstring MorphName;
		/// \brief The amount of frames until the next keyframe
		uint32_t FrameCount;
		/// \brief The weight of the morph to be applied
		float Weight;
	};

	/// \brief Stores information about camera movement
	struct CameraKeyFrame
	{
		/// \brief The amount of frames until the next keyframe
		uint32_t FrameCount;
		/// \brief The distance of the camera to the focal point
		float Distance;
		/// \brief The focal point
		btVector3 Position;
		/// \brief Rotation around the focal point
		btVector3 Angles;
		/// \brief Animation interpolation data
		std::vector<float> InterpolationData[6];
		/// \brief Field of View angle, in radians
		float FovAngle;
		/// \brief Use or not orthogonal projection
		///
		/// \remarks If this is not zero, use orthogonal projection, if zero, use perspective projection
		uint8_t NoPerspective;
	};

}
