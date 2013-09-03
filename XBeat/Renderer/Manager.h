#pragma once

#include <Windows.h>
#include <memory>

#include "D3DRenderer.h"
#include "CameraClass.h"
#include "Model.h"
#include "Shaders/LightShader.h"
#include "Light.h"
#include "ViewFrustum.h"
#include "D3DTextureRenderer.h"
#include "../Input/Manager.h"
#include "OrthoWindowClass.h"
#include "Shaders/Texture.h"
#include "Shaders/MMDEffect.h"

namespace Renderer {

const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;

class Manager
{
public:
	Manager();
	virtual ~Manager();

	bool Initialize(int width, int height, HWND wnd, std::shared_ptr<Input::Manager> input);
	void Shutdown();
	bool Frame(int mouseX, int mouseY);

private:
	bool Render(float frameTime);
	bool RenderToTexture();
	bool RenderScene();
	bool RenderEffects();
	bool Render2DTextureScene();

	float frameMsec();

	int lastMouseX, lastMouseY;
	int screenWidth, screenHeight;

	HWND wnd;

	std::shared_ptr<ViewFrustum> frustum;
	std::shared_ptr<D3DRenderer> d3d;
	std::shared_ptr<CameraClass> camera;
	std::shared_ptr<Model> model;
	std::shared_ptr<Model> stage;
	std::shared_ptr<Light> light;
	std::shared_ptr<Shaders::Light> lightShader;
	std::shared_ptr<Shaders::Texture> textureShader;
	std::shared_ptr<Shaders::MMDEffect> mmdEffect;
	std::shared_ptr<Input::Manager> input;
	std::shared_ptr<D3DTextureRenderer> renderTexture;
	std::shared_ptr<OrthoWindowClass> fullWindow;
};

}
