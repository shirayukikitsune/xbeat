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

#include <Urho3D/Urho3D.h>
#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/Vector.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/Vector3.h>

namespace VMD {

	/// \brief Stores information about the bone animation
	struct BoneKeyFrame
	{
		/// \brief The name of the bone to be animated
		Urho3D::String BoneName;
		/// \brief The amount of frames until the next keyframe
		unsigned int FrameCount;
		/// \brief The bone translation component
		Urho3D::Vector3 Translation;
		/// \brief The bone rotation component, a quaternion
		Urho3D::Quaternion Rotation;
		/// \brief Bone interpolation data
		Urho3D::PODVector<float> InterpolationData[4];
	};

	/// \brief Stores information about morph animations
	struct MorphKeyFrame
	{
		/// \brief The name of the morph to be applied, Shift-JIS encoded
		Urho3D::String MorphName;
		/// \brief The amount of frames until the next keyframe
		unsigned int FrameCount;
		/// \brief The weight of the morph to be applied
		float Weight;
	};

	/// \brief Stores information about camera movement
	struct CameraKeyFrame
	{
		/// \brief The amount of frames until the next keyframe
		unsigned int FrameCount;
		/// \brief The distance of the camera to the focal point
		float Distance;
		/// \brief The focal point
		Urho3D::Vector3 Position;
		/// \brief Rotation around the focal point
		Urho3D::Quaternion Rotation;
		/// \brief Animation interpolation data
		Urho3D::PODVector<float> InterpolationData[6];
		/// \brief Field of View angle, in radians
		float FovAngle;
		/// \brief Use or not orthogonal projection
		///
		/// \remarks If this is not zero, use orthogonal projection, if zero, use perspective projection
		unsigned char NoPerspective;
	};

}
