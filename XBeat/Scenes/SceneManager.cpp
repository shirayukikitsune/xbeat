#include "SceneManager.h"

#include "../ModelManager.h"
#include "../Scenes/LoadingScene.h"
#include "../Scenes/MenuScene.h"
#include "../VMD/MotionController.h"
#include "../Renderer/D3DRenderer.h"
#include "../Renderer/Shaders/PostProcessEffect.h"
#include "../Renderer/Shaders/Texture.h"

#include <iomanip>
#include <sstream>

Scenes::SceneManager::SceneManager()
{
}


Scenes::SceneManager::~SceneManager()
{
	shutdown();
}

bool Scenes::SceneManager::initialize(int ScreenWidth, int ScreenHeight, HWND WindowHandle, std::shared_ptr<Input::Manager> InputManager, std::shared_ptr<Physics::Environment> PhysicsEnvironment, std::shared_ptr<Dispatcher> EventDispatcher)
{
	this->InputManager = InputManager;
	this->PhysicsEnvironment = PhysicsEnvironment;
	this->EventDispatcher = EventDispatcher;
	this->WindowHandle = WindowHandle;

	Renderer.reset(new Renderer::D3DRenderer);
	assert(Renderer);

	if (!Renderer->Initialize(ScreenWidth, ScreenHeight, Renderer::VSYNC_ENABLED, WindowHandle, Renderer::FULL_SCREEN, Renderer::SCREEN_DEPTH, Renderer::SCREEN_NEAR)) {
		MessageBox(WindowHandle, L"Failed to initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	ModelHandler.reset(new ModelManager);
	assert(ModelHandler);

	TextureShader.reset(new Renderer::Shaders::Texture);
	assert(TextureShader);

	if (!TextureShader->Initialize(Renderer->GetDevice(), WindowHandle)) {
		MessageBox(WindowHandle, L"Could not initialize the texture shader object", L"Error", MB_OK);
		return false;
	}

	PostProcessShader.reset(new Renderer::Shaders::PostProcessEffect);
	assert(PostProcessShader);

	if (!PostProcessShader->Initialize(Renderer->GetDevice(), WindowHandle, ScreenWidth, ScreenHeight)) {
		MessageBox(WindowHandle, L"Could not initialize the effects object", L"Error", MB_OK);
		return false;
	}

	TextureRenderer.reset(new Renderer::D3DTextureRenderer);
	assert(TextureRenderer);

	if (!TextureRenderer->Initialize(Renderer->GetDevice(), ScreenWidth, ScreenHeight, Renderer::SCREEN_DEPTH, Renderer::SCREEN_NEAR))
		return false;

	OrthoWindow.reset(new Renderer::OrthoWindowClass);
	assert(OrthoWindow);

	if (!OrthoWindow->Initialize(Renderer->GetDevice(), ScreenWidth, ScreenHeight))
		return false;

	SpriteBatch.reset(new DirectX::SpriteBatch(Renderer->GetDeviceContext()));
	assert(SpriteBatch);
	Font.reset(new DirectX::SpriteFont(Renderer->GetDevice(), L"./Data/Fonts/unifont.spritefont"));
	assert(Font);

#ifndef DEBUG
	SetWindowText(WindowHandle, TEXT("XBeat"));
#endif

	NextScene.reset(new Scenes::Menu);
	assert(NextScene);
	NextScene->setResources(EventDispatcher, ModelHandler, InputManager, PhysicsEnvironment, Renderer);

	// This is the task that will be executed when the loading screen is being shown.
	std::packaged_task<bool()> WaitTask([this] {
		// Load the model list
		this->ModelHandler->loadList();

		this->NextScene->initialize();

		return true;
	});

	CurrentScene.reset(new Scenes::Loading(WaitTask.get_future()));
	assert(CurrentScene);

	CurrentScene->setResources(EventDispatcher, ModelHandler, InputManager, PhysicsEnvironment, Renderer);
	if (!CurrentScene->initialize())
		return false;

	std::thread(std::move(WaitTask)).detach();

	return true;
}

void Scenes::SceneManager::shutdown()
{
	if (NextScene) {
		NextScene->shutdown();
		NextScene.reset();
	}
	if (CurrentScene) {
		CurrentScene->shutdown();
		CurrentScene.reset();
	}

	Font.reset();
	SpriteBatch.reset();

	if (OrthoWindow != nullptr) {
		OrthoWindow->Shutdown();
		OrthoWindow.reset();
	}

	if (TextureRenderer) {
		TextureRenderer->Shutdown();
		TextureRenderer.reset();
	}

	PostProcessShader.reset();
	TextureShader.reset();
	ModelHandler.reset();

	if (Renderer != nullptr) {
		Renderer->Shutdown();
		Renderer.reset();
	}

	PhysicsEnvironment.reset();
	InputManager.reset();
	ModelHandler.reset();
}

bool Scenes::SceneManager::runFrame(float FrameTime)
{
#ifdef DEBUG
	wchar_t Title[512];
	swprintf_s<512>(Title, L"XBeat - Frame Time: %.3fms - FPS: %.1f", FrameTime * 1000.0f, 1.0f / FrameTime);
	SetWindowText(WindowHandle, Title);
#endif

	if (CurrentScene && CurrentScene->isFinished()) {
		// No need to call Scene::shutdown, CurrentScene will be deleted when this move occurs
		CurrentScene = std::move(NextScene);
		NextScene.reset();
	}

	if (CurrentScene) CurrentScene->frame(FrameTime);

	if (!render(FrameTime))
		return false;

	return true;
}

bool Scenes::SceneManager::render(float FrameTime)
{
	if (!renderToTexture(FrameTime))
		return false;

	if (!renderEffects(FrameTime))
		return false;

	if (!render2DTextureScene(FrameTime))
		return false;

	return true;
}

bool Scenes::SceneManager::renderToTexture(float FrameTime)
{
	TextureRenderer->SetRenderTarget(Renderer->GetDeviceContext(), Renderer->GetDepthStencilView());

	TextureRenderer->ClearRenderTarget(Renderer->GetDeviceContext(), Renderer->GetDepthStencilView(), 0.0f, 0.0f, 0.0f, 1.0f);

	if (!renderScene(FrameTime))
		return false;

	Renderer->SetBackBufferRenderTarget();

	return true;
}

bool Scenes::SceneManager::renderScene(float FrameTime)
{
	if (CurrentScene && !CurrentScene->render()) return false;

	return true;
}

bool Scenes::SceneManager::renderEffects(float FrameTime)
{
	if (!PostProcessShader->Render(Renderer, OrthoWindow->GetIndexCount(), TextureRenderer, OrthoWindow, Renderer::SCREEN_DEPTH, Renderer::SCREEN_NEAR))
		return false;

	return true;
}

bool Scenes::SceneManager::render2DTextureScene(float frameTime)
{
	Renderer->SetBackBufferRenderTarget();
	Renderer->BeginScene(1.0f, 1.0f, 1.0f, 1.0f);

	Renderer->Begin2D();

	OrthoWindow->Render(Renderer->GetDeviceContext());

	if (!TextureShader->Render(Renderer->GetDeviceContext(), OrthoWindow->GetIndexCount(), PostProcessShader->GetCurrentOutputView()))
		return false;

	Renderer->End2D();

	// Render UI artifacts
	SpriteBatch->Begin();
	std::wstringstream ss;
	ss.precision(1);
	ss << L"FPS: " << std::fixed << 1.0f / frameTime << L" - " << frameTime * 1000.0f << L"ms";
	Font->DrawString(SpriteBatch.get(), ss.str().c_str(), DirectX::XMFLOAT2(9.0f, 9.0f), DirectX::Colors::Black, 0, DirectX::XMFLOAT2(0, 0), 1.0f);
	Font->DrawString(SpriteBatch.get(), ss.str().c_str(), DirectX::XMFLOAT2(11.0f, 9.0f), DirectX::Colors::Black, 0, DirectX::XMFLOAT2(0, 0), 1.0f);
	Font->DrawString(SpriteBatch.get(), ss.str().c_str(), DirectX::XMFLOAT2(9.0f, 11.0f), DirectX::Colors::Black, 0, DirectX::XMFLOAT2(0, 0), 1.0f);
	Font->DrawString(SpriteBatch.get(), ss.str().c_str(), DirectX::XMFLOAT2(11.0f, 11.0f), DirectX::Colors::Black, 0, DirectX::XMFLOAT2(0, 0), 1.0f);
	Font->DrawString(SpriteBatch.get(), ss.str().c_str(), DirectX::XMFLOAT2(10.0f, 10.0f), DirectX::Colors::Yellow, 0, DirectX::XMFLOAT2(0, 0), 1.0f);
	SpriteBatch->End();

	Renderer->EndScene();

	return true;
}
