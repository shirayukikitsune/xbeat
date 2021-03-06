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

#include "Scene.h"

#include <future>

namespace Renderer {
	class OrthoWindowClass;
	class Texture;
	namespace Shaders { class Texture; }
}

namespace Scenes {

	class Loading
		: public Scene
	{
	public:
		Loading(std::future<bool> &&Task);
		virtual ~Loading();

		virtual bool initialize();

		virtual void shutdown();

		virtual void frame(float FrameTime);

		virtual bool render();

		virtual bool isFinished();

	private:
		std::future<bool> LoadingTask;
		std::unique_ptr<Renderer::OrthoWindowClass> Window;
		std::unique_ptr<Renderer::Texture> Texture;
		std::unique_ptr<Renderer::Shaders::Texture> TextureShader;
	};

}
