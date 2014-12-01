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

#include "Scene.h"

#include <random>
#include <string>
#include <vector>

namespace PMX {
	class Model;
	class PMXShader;
}
namespace Renderer {
	class Camera;
	class ViewFrustum;
}
namespace VMD { class Motion; }

namespace Scenes {

	class Menu
		: public Scene
	{
	public:
		Menu();
		virtual ~Menu();

		virtual bool initialize();

		virtual void shutdown();

		virtual void frame(float FrameTime);

		virtual bool render();

		virtual bool isFinished();

		virtual void onAttached();

		virtual void onDeattached();

	private:
		std::unique_ptr<VMD::Motion> Motion;
		std::unique_ptr<Renderer::Camera> Camera;
		std::shared_ptr<Renderer::ViewFrustum> Frustum;

		std::vector<std::wstring> KnownMotions;

		std::shared_ptr<PMX::Model> Model;
		std::shared_ptr<PMX::PMXShader> Shader;

		std::mt19937 RandomGenerator;
		float WaitTime;
		bool Paused;
	};

}