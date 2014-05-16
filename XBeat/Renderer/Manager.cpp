#include "Manager.h"
#include "PMX/PMXModel.h"
#include "PMX/PMXBone.h"
#include "OBJ/OBJModel.h"

#include <iomanip>
#include <sstream>

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

bool Manager::Initialize(int width, int height, HWND wnd, std::shared_ptr<Input::Manager> input, std::shared_ptr<Physics::Environment> physics, std::shared_ptr<Dispatcher> dispatcher)
{
	this->input = input;
	this->physics = physics;
	m_dispatcher = dispatcher;

	d3d.reset(new D3DRenderer);
	if (d3d == nullptr)
		return false;

	if (!d3d->Initialize(width, height, VSYNC_ENABLED, wnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR)) {
		MessageBox(wnd, L"Failed to initialize Direct3D", L"Error", MB_OK);
		return false;
	}

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

	//light->SetAmbientColor(0.9f, 0.5f, 0.0f, 1.0f);
	light->SetAmbientColor(1.0f, 1.0f, 1.0f, 1.0f);
	light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
	light->SetSpecularColor(1.0f, 1.0f, 1.0f, 1.0f);
	light->SetDirection(0.0f, 0.0f, 1.0f);
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

	sky.reset(new SkyBox);
	if (sky == nullptr)
		return false;

	if (!sky->Initialize(d3d, L"./Data/Textures/Sky/violentdays.dds", light, wnd))
		return false;

	m_batch.reset(new DirectX::SpriteBatch(d3d->GetDeviceContext()));
	m_font.reset(new DirectX::SpriteFont(d3d->GetDevice(), L"./Data/Fonts/unifont.spritefont"));

	// Setup bindings
	input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_Z), [this](void* param) {
		model.get()->ApplyMorph(L"ﾍﾞｰﾙ非表示1", 1.0f);
		model.get()->ApplyMorph(L"ﾍﾞｰﾙ非表示2", 1.0f);
		model.get()->ApplyMorph(L"薔薇非表示", 1.0f);
		model.get()->ApplyMorph(L"肩服非表示", 1.0f);
		model.get()->ApplyMorph(L"眼帯off", 1.0f);
	});
	input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_S), [this](void* param) { model->ApplyMorph(L"翼羽ばたき", 0.9f); });
	input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_D), [this](void* param) { model->GetBoneByName(L"全ての親")->Translate(btVector3(0.0f, 1.0f, 0.0f)); });
	input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyPressed, DIK_F), [this](void* param) { model->GetBoneByName(L"右腕")->Rotate(btVector3(0.0f, 1.0f, 0.0f)); });
	//input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_F), [this](void* param) { model->GetBoneByName(L"右腕")->Rotate(btVector3(-1.0f, 0.0f, 0.0f)); });
	input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnMouseUp, 0), [this](void* param) { model->ApplyMorph(L"purple", 1.0f); });
	input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_1), [this](void* param) { model->ToggleDebugFlags(PMX::Model::DebugFlags::RenderBones); });
	input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_2), [this](void* param) { model->ToggleDebugFlags(PMX::Model::DebugFlags::RenderJoints); });
	input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_3), [this](void* param) { model->ToggleDebugFlags(PMX::Model::DebugFlags::RenderRigidBodies); });
	input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_4), [this](void* param) { model->ToggleDebugFlags(PMX::Model::DebugFlags::RenderSoftBodies); });
	input->AddBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_0), [this](void* param) { model->ToggleDebugFlags(PMX::Model::DebugFlags::DontRenderModel); });
	input->SetMouseBinding([this](std::shared_ptr<Input::MouseMovement> data) {
		static float rotation[2] = { 0.0f, 0.0f };
		if (data->x != 0 || data->y != 0) {
			rotation[0] += data->x / 20.0f;
			rotation[1] += data->y / 20.0f;
			auto bone = model->GetBoneByName(L"首");
			bone->Rotate(btVector3(0.0f, rotation[0], -rotation[1]));
			//btVector3 v = camera->GetRotation();
			//camera->SetRotation(v.getX() + rotation[1], v.getY() + rotation[0], 0.0f);
		}
	});

	screenWidth = width;
	screenHeight = height;

	this->wnd = wnd;

	return true;
}

bool Manager::LoadScene() {
	camera.reset(new CameraClass);
	if (camera == nullptr)
		return false;

	stage.reset(new OBJModel);
	if (!stage->Initialize(d3d, L"./Data/Models/flatground.txt", physics, m_dispatcher)) {
		MessageBox(wnd, L"Failed to initialize the stage object", L"Error", MB_OK);
		return false;
	}

	model.reset(new PMX::Model);
	//if (!model->Initialize(d3d, L"./Data/Models/Tda式ミクAP改変 モリガン風 Ver106 配布用/Tda式ミクAP改変 リリス風 Ver106.pmx", physics, m_dispatcher)) {
	//if (!model->Initialize(d3d, L"./Data/Models/Tda式改変GUMI フェリシア風 Ver101 配布用/Tda式改変GUMI デフォ服 Ver101.pmx", physics, m_dispatcher)) {
	//if (!model->Initialize(d3d, L"./Data/Models/2013RacingMikuMMD/2013RacingMiku.pmx", physics, m_dispatcher)) {
	//if (!model->Initialize(d3d, L"./Data/Models/TDA IA Amulet/TDA IA Amulet.pmx", physics, m_dispatcher)) {
	if (!model->Initialize(d3d, L"./Data/Models/銀獅式波音リツ_レクイエム_ver1.20/銀獅式波音リツ_レクイエム_ver1.20.pmx", physics, m_dispatcher)) {
	//if (!model->Initialize(d3d, L"./Data/Models/SeeU 3.5/SeeU 3.5.pmx", physics, m_dispatcher)) {
	//if (!model->Initialize(d3d, L"./Data/Models/Tda China IA by SapphireRose-chan/Tda China IA v2.pmx", physics, m_dispatcher)) {
		MessageBox(wnd, L"Could not initialize the model object", L"Error", MB_OK);
		return false;
	}

	//auto pos = model->GetBoneByName(L"頭")->GetPosition();
	//camera->SetPosition(pos.x(), pos.y(), pos.z());
	camera->SetPosition(0.0f, 10.0f, -30.f);

	return true;
}

void Manager::Shutdown()
{
	if (sky != nullptr) {
		sky->Shutdown();
		sky.reset();
	}

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

	if (physics != nullptr) {
		physics.reset();
	}

	if (input != nullptr) {
		input.reset();
	}
}

bool Manager::Frame(float frameTime)
{
	static float lightDirection[2] = { 0.0f, 1.0f };
	static bool animating = false, increase = true;
	static float value = 0.0f;
	static float totalTime = 0.0f, animTime = 0.0f;

	wchar_t title[512];
	swprintf_s<512>(title, L"XBeat - Frame Time: %.3fms - FPS: %.1f", frameTime, 1000.0f / (float)frameTime);
	SetWindowText(this->wnd, title);

	totalTime += frameTime;
	//lightDirection[0] = sinf(3.1415f * 2.0f * totalTime / 10000.f);
	//lightDirection[1] = cosf(3.1415f * 2.0f * totalTime / 10000.f);
	//lightDirection[0] += msec / 1000.f;
	//lightDirection[1] += msec / 1000.f;

	//light->SetDirection(lightDirection[0], 0.0f, lightDirection[1]);

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
	//else { model.get()->GetBoneByName(L"右腕")->Rotate(btVector3(0.0f, 0.0f, 0.0f)); }
	/*if (input->IsKeyPressed(DIK_S)) {
		model.get()->ApplyMorph(L"purple", 0.3f);
	}*/
	if (input->IsKeyPressed(DIK_A)) {
		if (!animating) {
			animTime = 0.0f;
			animating = true;
			increase = true;
		}

		if (increase) {
			animTime += frameTime;
		}
		else {
			animTime -= frameTime;
		}

		value = sinf(animTime / 50);
		//value = expf(animTime / 600.f) - 1;

		if (value >= 1.0f) {
			value = 1.0f;
			increase = false;
		}
		else if (value <= 0.f) {
			value = 0.f;
			increase = true;
		}

		model->ApplyMorph(L"翼羽ばたき", value);
	}
	else if (animating) {
		//value = 0.0f;
		animating = false;
		//model->ApplyMorph(L"翼羽ばたき", value);
	}

	camera->SetPosition(campos.x, campos.y, campos.z);
	camera->Render();

	if (!Render(frameTime))
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

	if (!Render2DTextureScene(frameTime))
		return false;

	return true;
}

bool Manager::RenderToTexture()
{
	renderTexture->SetRenderTarget(d3d->GetDeviceContext(), d3d->GetDepthStencilView());

	renderTexture->ClearRenderTarget(d3d->GetDeviceContext(), d3d->GetDepthStencilView(), 0.0f, 0.0f, 0.0f, 1.0f);

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

	if (!sky->Render(d3d, world, view, projection, camera, light))
		return false;

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

bool Manager::Render2DTextureScene(float frameTime)
{
	DirectX::XMMATRIX view, ortho, world;

	camera->GetViewMatrix(view);
	d3d->GetWorldMatrix(world);
	d3d->GetOrthoMatrix(ortho);

	d3d->SetBackBufferRenderTarget();
	d3d->BeginScene(1.0f, 1.0f, 1.0f, 1.0f);

	d3d->Begin2D();

	fullWindow->Render(d3d->GetDeviceContext());

	if (!textureShader->Render(d3d->GetDeviceContext(), fullWindow->GetIndexCount(), world, view, ortho, mmdEffect->GetCurrentOutputView()))
		return false;

	d3d->End2D();

	// Render UI artifacts
	m_batch->Begin();
	std::wstringstream ss;
	ss << L"FPS: " << std::setprecision(3) << 1000.0f / frameTime << L" - " << frameTime << L"ms";
	m_font->DrawString(m_batch.get(), ss.str().c_str(), DirectX::XMFLOAT2(10.0f, 10.0f), DirectX::Colors::Yellow);
	m_batch->End();

	d3d->EndScene();

	return true;
}
