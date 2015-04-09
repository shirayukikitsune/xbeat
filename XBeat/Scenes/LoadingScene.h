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

#include <Ptr.h>

#include <future>

namespace Urho3D {
	class Context;
	class Scene;
	class Node;
}

namespace Scenes {

	class Loading
	{
	public:
		Loading(Urho3D::Context *Context, std::future<bool> &&Task);

		void initialize();

		bool isFinished();

	private:
		Urho3D::Context *Context;

		Urho3D::SharedPtr<Urho3D::Scene> Scene;
		Urho3D::SharedPtr<Urho3D::Node> CameraNode;

		std::future<bool> LoadingTask;
	};

}
