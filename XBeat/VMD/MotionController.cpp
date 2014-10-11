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

using namespace VMD;

MotionController::MotionController()
{
}


MotionController::~MotionController()
{
}

void MotionController::advanceFrame(float Time)
{

}

std::shared_ptr<Motion> MotionController::loadMotion(std::wstring FileName)
{
	return nullptr;
}
