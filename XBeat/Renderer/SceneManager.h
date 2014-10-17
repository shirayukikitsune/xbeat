#pragma once

#include <Windows.h>
#include <memory>

#include "SpriteFont.h"

class Dispatcher;
class ModelManager;
namespace Input { class Manager; }
namespace Physics { class Environment; }
namespace VMD { class MotionController; }

namespace Renderer {
	class D3DRenderer;
	class D3DTextureRenderer;
	class OrthoWindowClass;
	class Scene;
	namespace Shaders {
		class Texture;
		class PostProcessEffect;
	}

	const bool FULL_SCREEN = false;
	const bool VSYNC_ENABLED = false;
	const float SCREEN_DEPTH = 500.0f;
	const float SCREEN_NEAR = 0.25f;

	class SceneManager
	{
	public:
		SceneManager();
		virtual ~SceneManager();

		bool Initialize(int width, int height, HWND wnd, std::shared_ptr<Input::Manager> input, std::shared_ptr<Physics::Environment> physics, std::shared_ptr<Dispatcher> dispatcher);
		void Shutdown();
		bool Frame(float frameTime);

	private:
		bool Render(float frameTime);
		bool RenderToTexture(float frameTime);
		bool RenderScene(float frameTime);
		bool RenderEffects(float frameTime);
		bool Render2DTextureScene(float frameTime);

		int screenWidth, screenHeight;

		HWND wnd;

		std::unique_ptr<DirectX::SpriteFont> m_font;
		std::unique_ptr<DirectX::SpriteBatch> m_batch;
		std::unique_ptr<VMD::MotionController> MotionManager;

		std::unique_ptr<Scene> CurrentScene;
		std::unique_ptr<Scene> NextScene;

		std::shared_ptr<ModelManager> m_modelManager;

		std::shared_ptr<D3DRenderer> d3d;
		std::shared_ptr<Shaders::Texture> textureShader;
		std::shared_ptr<Shaders::PostProcessEffect> m_postProcess;
		std::shared_ptr<Input::Manager> input;
		std::shared_ptr<Physics::Environment> physics;
		std::shared_ptr<D3DTextureRenderer> renderTexture;
		std::shared_ptr<OrthoWindowClass> fullWindow;
		std::shared_ptr<Dispatcher> m_dispatcher;
	};

}
