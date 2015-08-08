//===-- Scenes/MenuScene.h - Declares the class for the menu interaction ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the menu scene class 
///
//===----------------------------------------------------------------------------------===//

#pragma once

#include "../VMD/Motion.h"

#include <Urho3D/Urho3D.h>
#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Core/Variant.h>

#include <future>
#include <random>

namespace Urho3D {
	class Context;
	class Scene;
	class Node;
}

namespace Scenes {

	class Menu
		: public Urho3D::Object
	{
		OBJECT(Scenes::Menu);

	public:
		Menu(Urho3D::Context *Context);
		~Menu();

		void initialize();

		Urho3D::SharedPtr<Urho3D::Scene> getScene() { return Scene; }

	private:
		Urho3D::SharedPtr<Urho3D::Scene> Scene;
		Urho3D::SharedPtr<Urho3D::Node> CameraNode;

		Urho3D::SharedPtr<VMD::Motion> Motion;

		Urho3D::Vector<Urho3D::String> KnownMotions;

		std::mt19937_64 RandomGenerator;

		float waitTime;

		void HandleSceneUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
	};

}
