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

#include <boost/filesystem/path.hpp>
#include <memory>
#include <string>
#include <vector>

namespace VMD {

	class Motion;

	/// \brief Class that manages all running VMD motions
	class MotionController
	{
	public:
		MotionController();
		~MotionController();
		
		/// \brief Advances all motions by the specified amount of time
		///
		/// \param [in] Time The amount of time, in milliseconds to advance the VMD motions
		void advanceFrame(float Time);

		/// \brief Loads a VMD motion from the specified filename
		///
		/// \param [in] FileName The file to load the motion from
		std::shared_ptr<Motion> loadMotion(std::wstring FileName);

	private:
		/// \brief Stores all loaded motions
		std::vector<std::shared_ptr<Motion>> KnownMotions;
	};

}
