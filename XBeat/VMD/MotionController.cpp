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

#include <Context.h>
#include <ResourceCache.h>
#include <Variant.h>

using namespace VMD;

MotionController::MotionController(Urho3D::Context *context)
	: Urho3D::LogicComponent(context)
	, motionsAttr(Motion::GetTypeStatic())
{
	FramesPerSecond = 30.f;

	SetUpdateEventMask(Urho3D::USE_UPDATE);
}

MotionController::~MotionController()
{
}

void MotionController::RegisterObject(Urho3D::Context* context)
{
	using namespace Urho3D;
	context->RegisterFactory<MotionController>();

	ATTRIBUTE(VMD::MotionController, VAR_FLOAT, "FPS", FramesPerSecond, 30.0f, AM_DEFAULT);
}

void MotionController::Update(float timeStep)
{
	float FrameCount = timeStep * FramesPerSecond;

	for (auto motion = KnownMotions.Begin(); motion != KnownMotions.End(); ++motion) {
		(*motion)->advanceFrame(FrameCount);
	}
}

void MotionController::ClearFinished()
{
	for (auto motion = KnownMotions.Begin(); motion != KnownMotions.End(); ) {
		if ((*motion)->isFinished()) {
			KnownMotions.Erase(motion);
			motion = KnownMotions.Begin();
		}
		else ++motion;
	}
}

Urho3D::SharedPtr<Motion> MotionController::LoadMotion(Urho3D::String FileName)
{
	Urho3D::SharedPtr<Motion> Output;

	auto cache = GetSubsystem<Urho3D::ResourceCache>();
	Output = cache->GetResource<Motion>(FileName, false);
	if (Output.Null())
		return Output;

	KnownMotions.Push(Output);

	return Output;
}

void MotionController::SetMotionsAttr(const Urho3D::ResourceRefList& value)
{
	auto cache = GetSubsystem<Urho3D::ResourceCache>();
	for (unsigned i = 0; i < value.names_.Size(); ++i) {
		auto motion = cache->GetResource<Motion>(value.names_[i]);
		if (motion)
			KnownMotions.Push(Urho3D::SharedPtr<Motion>(motion));
	}
}

const Urho3D::ResourceRefList& MotionController::GetMotionsAttr() const
{
	motionsAttr.names_.Resize(KnownMotions.Size());
	for (unsigned i = 0; i < KnownMotions.Size(); ++i)
		motionsAttr.names_[i] = Urho3D::GetResourceName(KnownMotions[i]);

	return motionsAttr;
}

