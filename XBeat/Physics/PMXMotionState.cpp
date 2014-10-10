//===-- Physics/PMXMotionState.cpp - Defines a Motion State for PMX ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===-----------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines everything related to the Physics::PMXMotionState class,
/// which is an interface between bullet dynamics world and a PMX::Model.
///
//===-----------------------------------------------------------------------------===//

#include "PMXMotionState.h"

#include "../PMX/PMXBone.h"

Physics::PMXMotionState::PMXMotionState(PMX::Bone *AssociatedBone, const btTransform &InitialTransform)
{
	assert(AssociatedBone != nullptr);

	this->InitialTransform = InitialTransform;
	this->AssociatedBone = AssociatedBone;
}

void Physics::PMXMotionState::getWorldTransform(btTransform &WorldTransform) const
{
	WorldTransform = InitialTransform * AssociatedBone->GetLocalTransform();
}

void Physics::PMXMotionState::setWorldTransform(const btTransform &WorldTransform)
{
}
