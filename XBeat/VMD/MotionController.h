//===-- VMD/MotionController.h - Declares the VMD motion manager --*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the VMD::MotionController
/// class
///
//===------------------------------------------------------------------------===//

#pragma once

#include "Motion.h"

#include <LogicComponent.h>
#include <Ptr.h>
#include <Str.h>
#include <Vector.h>

namespace VMD {

	/// \brief Class that manages all running VMD motions
	class MotionController
		: public Urho3D::LogicComponent
	{
		OBJECT(VMD::MotionController);

	public:
		MotionController(Urho3D::Context *context);
		virtual ~MotionController();
		static void RegisterObject(Urho3D::Context *context);
		
		/// \brief Advances all motions by the specified amount of time
		///
		/// \param [in] Time The amount of time, in milliseconds to advance the VMD motions
		virtual void Update(float timeStep);

		/// \brief Sets the new amount of Frames per Second
		void SetFPS(float FPS) { FramesPerSecond = FPS; }
		/// \brief Gets the amount of frames per second
		float GetFPS() const { return FramesPerSecond; }

		/// \brief Removes all finished motions
		void ClearFinished();

		/// \brief Loads a VMD motion from the specified filename
		///
		/// \param [in] FileName The file to load the motion from
		Urho3D::SharedPtr<Motion> LoadMotion(Urho3D::String FileName);

		/// Set motions attribute.
		void SetMotionsAttr(const Urho3D::ResourceRefList& value);
		/// Return motions attribute.
		const Urho3D::ResourceRefList& GetMotionsAttr() const;

	private:
		/// \brief Stores all loaded motions
		Urho3D::Vector<Urho3D::SharedPtr<Motion>> KnownMotions;

		/// \brief The amount of frames per second of the motions
		float FramesPerSecond;

		mutable Urho3D::ResourceRefList motionsAttr;
	};

}
