#include "Manager.h"
#include "PMX/PMXModel.h"
#include <boost/date_time.hpp>

using Renderer::Manager;

Manager::Manager()
{
	d3d.reset();
	camera.reset();
	model.reset();
	stage.reset();
	lightShader.reset();
	light.reset();
	frustum.reset();
}


Manager::~Manager()
{
	Shutdown();
}

bool Manager::Initialize(int width, int height, HWND wnd, std::shared_ptr<Input::Manager> input)
{
	this->input = input;
	d3d.reset(new D3DRenderer);
	if (d3d == nullptr)
		return false;

	if (!d3d->Initialize(width, height, VSYNC_ENABLED, wnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR)) {
		MessageBox(wnd, L"Failed to initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	camera.reset(new CameraClass);
	if (camera == nullptr)
		return false;
	camera->SetPosition(0.0f, 9.0f, -30.0f);

	stage.reset(new Model);
	if (!stage->Initialize(d3d->GetDevice(), L"./Data/Models/cube.txt")) {
		MessageBox(wnd, L"Failed to initialize the stage object", L"Error", MB_OK);
		return false;
	}

	model.reset(new PMX::Model);
	//if (!model->Initialize(d3d->GetDevice(), L"./Data/Models/Tda式ミクAP改変 モリガン風 Ver106 配布用/Tda式ミクAP改変 フェリシア風 Ver106.pmx")) {
	if (!model->Initialize(d3d->GetDevice(), L"./Data/Models/2013RacingMikuMMD/2013RacingMiku.pmx")) {
		MessageBox(wnd, L"Could not initialize the model object", L"Error", MB_OK);
		return false;
	}

	camera->SetPosition(0.0f, 10.0f, -30.f);

	lightShader.reset(new Shaders::Light);
	if (lightShader == nullptr)
		return false;

	if (!lightShader->Initialize(d3d->GetDevice(), wnd)) {
		MessageBox(wnd, L"Could not initialize the shader object", L"Error", MB_OK);
		return false;
	}

	textureShader.reset(new Shaders::Texture);
	if (textureShader == nullptr)
		return false;

	if (!textureShader->Initialize(d3d->GetDevice(), wnd)) {
		MessageBox(wnd, L"Could not initialize the texture shader object", L"Error", MB_OK);
		return false;
	}

	mmdEffect.reset(new Shaders::MMDEffect);
	if (mmdEffect == nullptr)
		return false;

	if (!mmdEffect->Initialize(d3d->GetDevice(), wnd, width, height)) {
		MessageBox(wnd, L"Could not initialize the effects object", L"Error", MB_OK);
		return false;
	}

	light.reset(new Light);
	if (light == nullptr)
		return false;

	light->SetAmbientColor(0.75f, 0.75f, 0.75f, 1.0f);
	light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
	light->SetSpecularColor(1.0f, 1.0f, 1.0f, 1.0f);
	light->SetDirection(1.0f, 0.0f, 1.0f);
	light->SetSpecularPower(32.0f);

	renderTexture.reset(new D3DTextureRenderer);
	if (renderTexture == nullptr)
		return false;

	if (!renderTexture->Initialize(d3d->GetDevice(), width, height, SCREEN_DEPTH, SCREEN_NEAR))
		return false;

	frustum.reset(new ViewFrustum);
	if (frustum == nullptr)
		return false;

	fullWindow.reset(new OrthoWindowClass);
	if (fullWindow == nullptr)
		return false;

	if (!fullWindow->Initialize(d3d->GetDevice(), width, height))
		return false;

	lastMouseX = lastMouseY = 0;
	screenWidth = width;
	screenHeight = height;

	this->wnd = wnd;

	return true;
}

void Manager::Shutdown()
{
	if (fullWindow != nullptr) {
		fullWindow->Shutdown();
		fullWindow.reset();
	}

	if (renderTexture != nullptr) {
		renderTexture->Shutdown();
		renderTexture.reset();
	}

	if (light != nullptr) {
		light.reset();
	}

	if (lightShader != nullptr) {
		lightShader->Shutdown();
		lightShader.reset();
	}

	if (model != nullptr) {
		model->Shutdown();
		model.reset();
	}

	if (stage != nullptr) {
		stage->Shutdown();
		stage.reset();
	}

	if (camera != nullptr) {
		camera.reset();
	}

	if (d3d != nullptr) {
		d3d->Shutdown();
		d3d.reset();
	}
}

bool Manager::Frame(int mouseX, int mouseY)
{
	static float rotation[2] = { 0.0f, 0.0f };
	static float lightDirection[2] = { 0.0f, 1.0f };
	POINT cursorpos;
	static bool animating = false, increase = true;
	static float value = 0.0f;
	static float totalTime = 0.0f, animTime = 0.0f;
	static float euler = expf(1.0f);

	float msec = frameMsec();

	wchar_t title[512];
	swprintf_s<512>(title, L"XBeat - Frame Time: %.3fms - FPS: %.1f", msec, 1000.0f / (float)msec);

	GetCursorPos(&cursorpos);
	int dx = cursorpos.x - GetSystemMetrics(SM_CXSCREEN) / 2;
	int dy = cursorpos.y - GetSystemMetrics(SM_CYSCREEN) / 2;
	SetCursorPos(GetSystemMetrics(SM_CXSCREEN) / 2, GetSystemMetrics(SM_CYSCREEN) / 2);

	SetWindowText(this->wnd, title);

	totalTime += msec;
	//rotation[0] += (float)dx / 5.0f;
	//rotation[1] += (float)dy / 5.0f;
	lightDirection[0] = sinf(3.1415f * 2.0f * totalTime / 10000.f);
	lightDirection[1] = cosf(3.1415f * 2.0f * totalTime / 10000.f);
	//lightDirection[0] += msec / 1000.f;
	//lightDirection[1] += msec / 1000.f;

	light->SetDirection(lightDirection[0], 0.0f, lightDirection[1]);

	DirectX::XMFLOAT3 campos;
	DirectX::XMStoreFloat3(&campos, camera->GetPosition());

	if (input->IsKeyPressed(DIK_UPARROW)) {
		if (input->IsKeyPressed(DIK_LCONTROL))
			campos.x += 0.1f;
		else if (!input->IsKeyPressed(DIK_LSHIFT))
			campos.y += 0.1f;
		else
			campos.z += 0.1f;
	}
	if (input->IsKeyPressed(DIK_DOWNARROW)) {
		if (input->IsKeyPressed(DIK_LCONTROL))
			campos.x -= 0.1f;
		else if (!input->IsKeyPressed(DIK_LSHIFT))
			campos.y -= 0.1f;
		else
			campos.z -= 0.1f;
	}
	if (input->IsKeyPressed(DIK_A)) { if (!animating) animTime = 0.0f; animating = true; /*((PMX::Model*)model.get())->ApplyMorph(L"猫目AL", 1.0f);*/ }
	else { value = 0.0f; animating = false; /*((PMX::Model*)model.get())->ApplyMorph(L"猫目AL", 0.0f);*/ }
	if (animating) {
		if (increase) {
			animTime += msec;
		}
		else {
			animTime -= msec;
		}

		value = expf(animTime / 600.f) - 1;
		
		if (value >= 1.0f) {
			value = 1.0f;
			increase = false;
		}
		else if (value <= 0.f) {
			value = 0.f;
			increase = true;
		}
	}

	((PMX::Model*)model.get())->ApplyMorph(L"ウィンク２", value);
	//((PMX::Model*)model.get())->ApplyMorph(L"ｳｨﾝｸ２右", value);
	//((PMX::Model*)model.get())->ApplyMorph(L"はぁと", value);

	camera->SetPosition(campos.x, campos.y, campos.z);
	camera->SetRotation(rotation[1], rotation[0], 0.0f);
	camera->Render();

	if (!Render(msec))
		return false;

	return true;
}

bool Manager::Render(float frameTime)
{
	if (!RenderToTexture())
		return false;

	/*if (!RenderScene())
		return false;*/

	if (!RenderEffects())
		return false;

	if (!Render2DTextureScene())
		return false;

	return true;
}

bool Manager::RenderToTexture()
{
	renderTexture->SetRenderTarget(d3d->GetDeviceContext(), d3d->GetDepthStencilView(), 0);

	renderTexture->ClearRenderTarget(d3d->GetDeviceContext(), d3d->GetDepthStencilView(), 0, 0.0f, 0.0f, 0.0f, 1.0f);

	if (!RenderScene())
		return false;

	d3d->SetBackBufferRenderTarget();

	return true;
}

bool Manager::RenderScene()
{
	DirectX::XMMATRIX view, projection, world;

	camera->GetViewMatrix(view);
	d3d->GetWorldMatrix(world);
	d3d->GetProjectionMatrix(projection);

	frustum->Construct(SCREEN_DEPTH, projection, view);

	if (!stage->Render(d3d, lightShader, view, projection, world, light, camera, frustum))
		return false;

	if (!model->Render(d3d, lightShader, view, projection, world, light, camera, frustum))
		return false;

	return true;
}

bool Manager::RenderEffects()
{
	DirectX::XMMATRIX view, ortho, world;

	camera->GetViewMatrix(view);
	d3d->GetWorldMatrix(world);
	d3d->GetOrthoMatrix(ortho);

	if (!mmdEffect->Render(d3d, fullWindow->GetIndexCount(), world, view, ortho, renderTexture, fullWindow, SCREEN_DEPTH, SCREEN_NEAR))
		return false;

	return true;
}

bool Manager::Render2DTextureScene()
{
	DirectX::XMMATRIX view, ortho, world;

	camera->GetViewMatrix(view);
	d3d->GetWorldMatrix(world);
	d3d->GetOrthoMatrix(ortho);

	d3d->SetBackBufferRenderTarget();
	d3d->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

	d3d->Begin2D();

	fullWindow->Render(d3d->GetDeviceContext());

	if (!textureShader->Render(d3d->GetDeviceContext(), fullWindow->GetIndexCount(), world, view, ortho, mmdEffect->GetCurrentOutputView()))
		return false;

	d3d->End2D();

	d3d->EndScene();

	return true;
}

float Manager::frameMsec()
{
	static boost::posix_time::ptime lastTime = boost::posix_time::microsec_clock::universal_time();

	boost::posix_time::ptime nowTime = boost::posix_time::microsec_clock::universal_time();
	float msec = (nowTime - lastTime).total_microseconds() / 1000.0f;
	lastTime = nowTime;

	return msec;
}
