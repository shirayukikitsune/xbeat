#pragma once

#include <Windows.h>
#include <memory>

#include "D3DRenderer.h"
#include "Camera.h"
#include "PMX/PMXModel.h"
#include "Model.h"
#include "Shaders/LightShader.h"
#include "Light.h"
#include "ViewFrustum.h"
#include "D3DTextureRenderer.h"
#include "../Input/InputManager.h"
#include "OrthoWindowClass.h"
#include "Shaders/Texture.h"
#include "Shaders/PostProcessEffect.h"
#include "../Physics/Environment.h"
#include "SkyBox.h"
#include "../Dispatcher.h"
#include "SpriteFont.h"
#include "PMX/PMXShader.h"

namespace Renderer {

const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = false;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.01f;

ATTRIBUTE_ALIGNED16(class) SceneManager
{
public:
	SceneManager();
	virtual ~SceneManager();

	bool Initialize(int width, int height, HWND wnd, std::shared_ptr<Input::Manager> input, std::shared_ptr<Physics::Environment> physics, std::shared_ptr<Dispatcher> dispatcher);
	bool LoadScene();
	void Shutdown();
	bool Frame(float frameTime);

	void LoadModel(std::wstring filename);

	__forceinline std::shared_ptr<D3DRenderer> GetRenderer() { return d3d; }

	BT_DECLARE_ALIGNED_ALLOCATOR();

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
	std::shared_ptr<ViewFrustum> frustum;
	std::shared_ptr<D3DRenderer> d3d;
	std::shared_ptr<SkyBox> sky;
	std::shared_ptr<Camera> camera;
	std::vector<std::shared_ptr<PMX::Model>> m_models;
	std::shared_ptr<Model> stage;
	std::shared_ptr<Light> light;
	std::shared_ptr<Shaders::Light> lightShader;
	std::shared_ptr<Shaders::Texture> textureShader;
	std::shared_ptr<Shaders::PostProcessEffect> m_postProcess;
	std::shared_ptr<PMX::PMXShader> m_pmxShader;
	std::shared_ptr<Input::Manager> input;
	std::shared_ptr<Physics::Environment> physics;
	std::shared_ptr<D3DTextureRenderer> renderTexture;
	std::shared_ptr<OrthoWindowClass> fullWindow;
	std::shared_ptr<Dispatcher> m_dispatcher;
};

}
