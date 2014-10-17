//===-- Scenes/SceneManager.h - Declares the class for magaging scenes ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the Scenes::SceneManager class 
///
//===--------------------------------------------------------------------------------===//

#pragma once

#include <Windows.h>
#include <memory>

#include "SpriteFont.h"

// Forward declarations
class Dispatcher;
class ModelManager;
namespace Input { class Manager; }
namespace Physics { class Environment; }
namespace VMD { class MotionController; }

namespace Renderer {
	class D3DRenderer;
	class D3DTextureRenderer;
	class OrthoWindowClass;
	namespace Shaders {
		class Texture;
		class PostProcessEffect;
	}

	/// \name Things that are going to be moved from here and placed in a settings class
	/// @{
	const bool FULL_SCREEN = false;
	const bool VSYNC_ENABLED = false;
	const float SCREEN_DEPTH = 500.0f;
	const float SCREEN_NEAR = 0.25f;
	/// @}
}

namespace Scenes {
	class Scene;

	/// \brief Class to manage all available scenes and how they interconnect
	class SceneManager
	{
	public:
		SceneManager();
		virtual ~SceneManager();

		/// \brief Initializes the SceneManager and
		///
		/// This also initializes some other used classes, like the DirectX renderer, the fonts and motion manager
		///
		/// \param [in] ScreenWidth The width of the DirectX viewport, in pixels
		/// \param [in] ScreenHeight The height of the DirectX viewport, in pixels
		/// \param [in] WindowHandle The HWND for a created window
		bool initialize(int ScreenWidth, int ScreenHeight, HWND WindowHandle, std::shared_ptr<Input::Manager> InputManager, std::shared_ptr<Physics::Environment> PhysicsEnvironment, std::shared_ptr<Dispatcher> EventDispatcher);
		/// \brief Destroys all created resources by the initialize() function
		void shutdown();
		/// \brief Runs a frame of the game
		///
		/// \param [in] FrameTime The duration of the last frame, in seconds
		bool runFrame(float FrameTime);

	private:
		/// \brief Initializes the rendering
		bool render(float FrameTime);
		/// \brief Prepare the rendering of the scene to a texture
		bool renderToTexture(float FrameTime);
		/// \brief Renders the scene
		bool renderScene(float FrameTime);
		/// \brief Render the post-process effect
		bool renderEffects(float FrameTime);
		/// \brief Render the post-processed scene
		bool render2DTextureScene(float FrameTime);

		/// \brief The handle of the main window
		HWND WindowHandle;

		/// \brief The font that may be used 
		std::unique_ptr<DirectX::SpriteFont> Font;
		/// \brief The sprite batch to render the font
		std::unique_ptr<DirectX::SpriteBatch> SpriteBatch;
		std::unique_ptr<VMD::MotionController> MotionManager;

		std::unique_ptr<Scene> CurrentScene;
		std::unique_ptr<Scene> NextScene;

		std::shared_ptr<ModelManager> ModelHandler;

		std::shared_ptr<Renderer::D3DRenderer> Renderer;
		std::unique_ptr<Renderer::Shaders::Texture> TextureShader;
		std::unique_ptr<Renderer::Shaders::PostProcessEffect> PostProcessShader;
		std::shared_ptr<Input::Manager> InputManager;
		std::shared_ptr<Physics::Environment> PhysicsEnvironment;
		std::shared_ptr<Renderer::D3DTextureRenderer> TextureRenderer;
		std::shared_ptr<Renderer::OrthoWindowClass> OrthoWindow;
		std::shared_ptr<Dispatcher> EventDispatcher;
	};

}
