//===-- VMD/MotionController.cpp - Defines the VMD motion manager --*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===-------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines everything related to the VMD::MotionController class
///
//===-------------------------------------------------------------------------===//

#include "MotionController.h"

#include "Motion.h"

using namespace VMD;

MotionController::MotionController()
{
}


MotionController::~MotionController()
{
}

void MotionController::advanceFrame(float Time)
{
	for (auto &Motion : KnownMotions) {
		Motion->advanceTime(Time);
	}
}

std::shared_ptr<Motion> MotionController::loadMotion(std::wstring FileName)
{
	std::shared_ptr<Motion> Output(new Motion);
	assert(Output);

	if (!Output->loadFromFile(FileName))
		return nullptr;

	KnownMotions.push_back(Output);

	return Output;
}
