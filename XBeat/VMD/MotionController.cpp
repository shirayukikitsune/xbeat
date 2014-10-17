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
	FramesPerSecond = 30.f;
}


MotionController::~MotionController()
{
}

void MotionController::advanceFrame(float Time)
{
	float FrameCount = Time * FramesPerSecond;

	for (auto &Motion : KnownMotions) {
		Motion->advanceFrame(FrameCount);
	}
}

void MotionController::clearFinished()
{
	for (auto motion = KnownMotions.begin(); motion != KnownMotions.end(); ) {
		if ((*motion)->isFinished()) {
			KnownMotions.erase(motion);
			motion = KnownMotions.begin();
		}
		else ++motion;
	}
}

void MotionController::setFPS(float FPS)
{
	FramesPerSecond = FPS;
}

float MotionController::getFPS()
{
	return FramesPerSecond;
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
