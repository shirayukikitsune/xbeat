#pragma once

#include <Windows.h>
#include <memory>

#include "D3DRenderer.h"
#include "CameraClass.h"
#include "PMX/PMXModel.h"
#include "Model.h"
#include "Shaders/LightShader.h"
#include "Light.h"
#include "ViewFrustum.h"
#include "D3DTextureRenderer.h"
#include "../Input/InputManager.h"
#include "OrthoWindowClass.h"
#include "Shaders/Texture.h"
#include "Shaders/MMDEffect.h"
#include "../Physics/Environment.h"
#include "SkyBox.h"
#include "../Dispatcher.h"
#include "SpriteFont.h"

namespace Renderer {

const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 500.0f;
const float SCREEN_NEAR = 0.25f;

ATTRIBUTE_ALIGNED16(class) Manager
{
public:
	Manager();
	virtual ~Manager();

	bool Initialize(int width, int height, HWND wnd, std::shared_ptr<Input::Manager> input, std::shared_ptr<Physics::Environment> physics, std::shared_ptr<Dispatcher> dispatcher);
	bool LoadScene();
	void Shutdown();
	bool Frame(float frameTime);

	__forceinline std::shared_ptr<D3DRenderer> GetRenderer() { return d3d; }

	BT_DECLARE_ALIGNED_ALLOCATOR();

private:
	bool Render(float frameTime);
	bool RenderToTexture();
	bool RenderScene();
	bool RenderEffects();
	bool Render2DTextureScene(float frameTime);

	int screenWidth, screenHeight;

	HWND wnd;

	std::unique_ptr<DirectX::SpriteFont> m_font;
	std::unique_ptr<DirectX::SpriteBatch> m_batch;
	std::shared_ptr<ViewFrustum> frustum;
	std::shared_ptr<D3DRenderer> d3d;
	std::shared_ptr<SkyBox> sky;
	std::shared_ptr<CameraClass> camera;
	std::shared_ptr<PMX::Model> model;
	std::shared_ptr<Model> stage;
	std::shared_ptr<Light> light;
	std::shared_ptr<Shaders::Light> lightShader;
	std::shared_ptr<Shaders::Texture> textureShader;
	std::shared_ptr<Shaders::MMDEffect> mmdEffect;
	std::shared_ptr<Input::Manager> input;
	std::shared_ptr<Physics::Environment> physics;
	std::shared_ptr<D3DTextureRenderer> renderTexture;
	std::shared_ptr<OrthoWindowClass> fullWindow;
	std::shared_ptr<Dispatcher> m_dispatcher;
};

}
