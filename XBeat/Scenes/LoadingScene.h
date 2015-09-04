//===-- Scenes/LoadingScene.h - Declares the class for the loading scene ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the loading scene class 
///
//===----------------------------------------------------------------------------------===//

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Scene/Scene.h>

namespace Urho3D {
	class Context;
	class Resource;
}

namespace Scenes {

	class LoadingScene
	{
	public:
		LoadingScene(Urho3D::Context *Context, Urho3D::Scene *nextScene);

		void initialize();

		bool isFinished();

	private:
		Urho3D::Context *Context;

		Urho3D::SharedPtr<Urho3D::Scene> NextScene;
	};

}
