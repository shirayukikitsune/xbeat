#include "SceneManager.h"

#include "../ModelManager.h"
#include "../Scenes/LoadingScene.h"
#include "../VMD/MotionController.h"
#include "D3DRenderer.h"
#include "Shaders/PostProcessEffect.h"
#include "Shaders/Texture.h"

#include <iomanip>
#include <sstream>

using Renderer::SceneManager;

SceneManager::SceneManager()
{
}


SceneManager::~SceneManager()
{
	Shutdown();
}

bool SceneManager::Initialize(int width, int height, HWND wnd, std::shared_ptr<Input::Manager> input, std::shared_ptr<Physics::Environment> physics, std::shared_ptr<Dispatcher> dispatcher)
{
	this->input = input;
	this->physics = physics;
	m_dispatcher = dispatcher;

	d3d.reset(new D3DRenderer);
	assert(d3d);

	if (!d3d->Initialize(width, height, VSYNC_ENABLED, wnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR)) {
		MessageBox(wnd, L"Failed to initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	m_modelManager.reset(new ModelManager);
	assert(m_modelManager);

	textureShader.reset(new Shaders::Texture);
	assert(textureShader);

	if (!textureShader->Initialize(d3d->GetDevice(), wnd)) {
		MessageBox(wnd, L"Could not initialize the texture shader object", L"Error", MB_OK);
		return false;
	}

	m_postProcess.reset(new Shaders::PostProcessEffect);
	assert(m_postProcess);

	if (!m_postProcess->Initialize(d3d->GetDevice(), wnd, width, height)) {
		MessageBox(wnd, L"Could not initialize the effects object", L"Error", MB_OK);
		return false;
	}

	renderTexture.reset(new D3DTextureRenderer);
	assert(renderTexture);

	if (!renderTexture->Initialize(d3d->GetDevice(), width, height, SCREEN_DEPTH, SCREEN_NEAR))
		return false;

	fullWindow.reset(new OrthoWindowClass);
	assert(fullWindow);

	if (!fullWindow->Initialize(d3d->GetDevice(), width, height))
		return false;

	m_batch.reset(new DirectX::SpriteBatch(d3d->GetDeviceContext()));
	m_font.reset(new DirectX::SpriteFont(d3d->GetDevice(), L"./Data/Fonts/unifont.spritefont"));

	screenWidth = width;
	screenHeight = height;

	this->wnd = wnd;

#ifndef DEBUG
	SetWindowText(this->wnd, TEXT("XBeat"));
#endif

	MotionManager.reset(new VMD::MotionController);
	assert(MotionManager);

	// This is the task that will be executed when the loading screen is being shown.
	std::packaged_task<bool()> WaitTask([this] {
		// Load the model list
		this->m_modelManager->loadList();

		return true;
	});

	CurrentScene.reset(new Scenes::Loading(WaitTask.get_future()));
	assert(CurrentScene);

	CurrentScene->setResources(m_dispatcher, d3d, m_modelManager, input, physics);
	if (!CurrentScene->initialize())
		return false;

	std::thread(std::move(WaitTask)).detach();

	return true;
}

void SceneManager::Shutdown()
{
	if (fullWindow != nullptr) {
		fullWindow->Shutdown();
		fullWindow.reset();
	}

	if (renderTexture != nullptr) {
		renderTexture->Shutdown();
		renderTexture.reset();
	}

	if (d3d != nullptr) {
		d3d->Shutdown();
		d3d.reset();
	}

	physics.reset();
	input.reset();
	m_modelManager.reset();
}

bool SceneManager::Frame(float frameTime)
{
#ifdef DEBUG
	wchar_t title[512];
	swprintf_s<512>(title, L"XBeat - Frame Time: %.3fms - FPS: %.1f", frameTime * 1000.0f, 1.0f / frameTime);
	SetWindowText(this->wnd, title);
#endif

	if (CurrentScene && CurrentScene->isFinished()) {
		CurrentScene = std::move(NextScene);
		NextScene.reset();
	}

	MotionManager->advanceFrame(frameTime);

	if (!Render(frameTime))
		return false;

	return true;
}

bool SceneManager::Render(float frameTime)
{
	if (!RenderToTexture(frameTime))
		return false;

	if (!RenderEffects(frameTime))
		return false;

	if (!Render2DTextureScene(frameTime))
		return false;

	return true;
}

bool SceneManager::RenderToTexture(float frameTime)
{
	renderTexture->SetRenderTarget(d3d->GetDeviceContext(), d3d->GetDepthStencilView());

	renderTexture->ClearRenderTarget(d3d->GetDeviceContext(), d3d->GetDepthStencilView(), 0.0f, 0.0f, 0.0f, 1.0f);

	if (!RenderScene(frameTime))
		return false;

	d3d->SetBackBufferRenderTarget();

	return true;
}

bool SceneManager::RenderScene(float frameTime)
{
	if (CurrentScene && !CurrentScene->render()) return false;

	return true;
}

bool SceneManager::RenderEffects(float frameTime)
{
	if (!m_postProcess->Render(d3d, fullWindow->GetIndexCount(), renderTexture, fullWindow, SCREEN_DEPTH, SCREEN_NEAR))
		return false;

	return true;
}

bool SceneManager::Render2DTextureScene(float frameTime)
{
	d3d->SetBackBufferRenderTarget();
	d3d->BeginScene(1.0f, 1.0f, 1.0f, 1.0f);

	d3d->Begin2D();

	fullWindow->Render(d3d->GetDeviceContext());

	if (!textureShader->Render(d3d->GetDeviceContext(), fullWindow->GetIndexCount(), m_postProcess->GetCurrentOutputView()))
		return false;

	d3d->End2D();

	// Render UI artifacts
	m_batch->Begin();
	std::wstringstream ss;
	ss.precision(1);
	ss << L"FPS: " << std::fixed << 1.0f / frameTime << L" - " << frameTime * 1000.0f << L"ms";
	m_font->DrawString(m_batch.get(), ss.str().c_str(), DirectX::XMFLOAT2(9.0f, 9.0f), DirectX::Colors::Black, 0, DirectX::XMFLOAT2(0, 0), 1.0f);
	m_font->DrawString(m_batch.get(), ss.str().c_str(), DirectX::XMFLOAT2(11.0f, 9.0f), DirectX::Colors::Black, 0, DirectX::XMFLOAT2(0, 0), 1.0f);
	m_font->DrawString(m_batch.get(), ss.str().c_str(), DirectX::XMFLOAT2(9.0f, 11.0f), DirectX::Colors::Black, 0, DirectX::XMFLOAT2(0, 0), 1.0f);
	m_font->DrawString(m_batch.get(), ss.str().c_str(), DirectX::XMFLOAT2(11.0f, 11.0f), DirectX::Colors::Black, 0, DirectX::XMFLOAT2(0, 0), 1.0f);
	m_font->DrawString(m_batch.get(), ss.str().c_str(), DirectX::XMFLOAT2(10.0f, 10.0f), DirectX::Colors::Yellow, 0, DirectX::XMFLOAT2(0, 0), 1.0f);
	m_batch->End();

	d3d->EndScene();

	return true;
}
