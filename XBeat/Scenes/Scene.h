//===-- Renderer/Scene.h - Declares a class for scene handling ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the scene class, which
/// takes responsibility for user output.
///
//===------------------------------------------------------------------------===//

#pragma once

#include <memory>

class Dispatcher;
class ModelManager;
namespace Input { class Manager; }
namespace Physics { class Environment; }
namespace Renderer { class D3DRenderer; }

namespace Scenes {
	/// \brief Class that is base to all user output functionalities
	class Scene
	{
	public:
		Scene();
		virtual ~Scene();

		/// \brief Set the pointers to the general resouce managers that this scene may have access to
		void setResources(std::shared_ptr<Dispatcher> EventDispatcher, std::shared_ptr<ModelManager> ModelHandler, std::shared_ptr<Input::Manager> InputManager, std::shared_ptr<Physics::Environment> Physics, std::shared_ptr<Renderer::D3DRenderer> Renderer);

		/// \brief Load all resources for this scene
		///
		/// \remarks When this is called, all shared resources are already set, so they can be used already
		virtual bool initialize() = 0;

		/// \brief Unload all resources for this scene
		///
		/// \remarks When this is called, it is unknown the state of the shared resources, so try to not use them
		virtual void shutdown() {}

		/// \brief Does pre-rendering tasks
		virtual void frame(float FrameTime) = 0;

		/// \brief Displays this scene
		virtual bool render() = 0;

		/// \brief Returns if this scene has done its job
		virtual bool isFinished() = 0;

		/// \brief Event fired when the current scene is switched to this
		virtual void onAttached() {}

		/// \brief Event fired when the current scene is switched out of this
		virtual void onDeattached() {}

	protected:
		/// \name Shared resources that might be used by each subclass
		/// @{
		std::shared_ptr<Dispatcher> EventDispatcher;
		std::shared_ptr<ModelManager> ModelHandler;
		std::shared_ptr<Input::Manager> InputManager;
		std::shared_ptr<Physics::Environment> Physics;
		std::shared_ptr<Renderer::D3DRenderer> Renderer;
		/// @}
	};
}
