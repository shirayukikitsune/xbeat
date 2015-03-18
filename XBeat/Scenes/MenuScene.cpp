//===-- Scenes/MenuScene.cpp - Defines the class for the menu interaction ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===-----------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines everything related to the menu scene class 
///
//===-----------------------------------------------------------------------------------===//

#include "MenuScene.h"

#include "SceneManager.h"
#include "../ModelManager.h"
#include "../Input/InputManager.h"
#include "../PMX/PMXModel.h"
#include "../PMX/PMXShader.h"
#include "../Renderer/Camera.h"
#include "../Renderer/D3DRenderer.h"
#include "../Renderer/ViewFrustum.h"
#include "../VMD/Motion.h"

#include <boost/filesystem.hpp>
#include <random>

namespace fs = boost::filesystem;

Scenes::Menu::Menu()
{
	std::random_device RandomDevice;
	RandomGenerator.seed(RandomDevice());
	Paused = false;
}

Scenes::Menu::~Menu()
{
}

bool Scenes::Menu::initialize()
{
	Shader.reset(new PMX::PMXShader);
	assert(Shader);
	if (!Shader->InitializeBuffers(Renderer->GetDevice(), nullptr))
		return false;
	Shader->SetLightCount(1);
	Shader->SetLights(DirectX::XMVectorSplatOne(), DirectX::XMVectorSplatOne(), DirectX::XMVectorSplatOne(), DirectX::XMVectorSet(-1.0f, -1.0f, 1.0f, 0.0f), DirectX::XMVectorZero(), 0);

	auto &Viewport = Renderer->getViewport();
	Camera.reset(new Renderer::Camera(DirectX::XM_PIDIV4, (float)Viewport.Width / (float)Viewport.Height, Renderer::SCREEN_NEAR, Renderer::SCREEN_DEPTH));
	assert(Camera);
	Camera->SetPosition(0, 10.0f, 0.0f);

	Frustum.reset(new Renderer::ViewFrustum);
	assert(Frustum);

	// Create a list of motions to be used as idle animations
	fs::directory_iterator EndIterator;
	fs::path MotionPath(L"./Data/Motions/Idle/");
	for (fs::directory_iterator Entry(MotionPath); Entry != EndIterator; Entry++) {
		if (fs::is_regular_file(Entry->status()) && Entry->path().extension().wstring().compare(L".vmd") == 0) {
			KnownMotions.emplace_back(Entry->path().generic_wstring());
		}
	}

	DirectX::XMMATRIX View, Projection;
	Camera->setFocalDistance(-35.0f);
	Camera->update(0.0f);
	Camera->getViewMatrix(View);
	Camera->getProjectionMatrix(Projection);

	Frustum->Construct(Renderer::SCREEN_DEPTH, Projection, View);

	Shader->SetEyePosition(Camera->GetPosition());
	Shader->SetMatrices(DirectX::XMMatrixIdentity(), View, Projection);

	// Initializes the model
	// Pause the physics environment, to prevent resource race
	Physics->pause();
	// Select a model to be displayed
#if 0
	auto ModelList = ModelHandler->getKnownModels();
	do {
		size_t Index = RandomGenerator() % ModelList.size();
		for (auto &ModelPath : ModelList) {
			if (Index-- == 0) {
				Model = ModelHandler->loadModel(ModelPath.first, Physics);
				ModelList.erase(ModelPath.first);
				break;
			}
		}
	} while (!Model);
#else
	Model = ModelHandler->loadModel(L"Tda式改変WIMミク ver.2.9", Physics);
	Model->SetDebugFlags(PMX::Model::DebugFlags::RenderBones);
#endif
	Model->SetShader(Shader);

	if (!Model->Initialize(Renderer, Physics))
		return false;
	Physics->resume();

	return true;
}

void Scenes::Menu::shutdown()
{
	Model.reset();
	Shader.reset();
	KnownMotions.clear();
}

void Scenes::Menu::frame(float FrameTime)
{
#if 1
	// Check if a new motion should be loaded
	if (!KnownMotions.empty()) {
		if (!Motion || Motion->isFinished()) {
			// Set the model to its initial state
			Model->Reset();

			// Add a waiting time 
			if (Motion != nullptr)
				WaitTime = (float)RandomGenerator() / (float)RandomGenerator.max() * 15.0f;

			// Initialize a new random VMD motion
			Motion.reset(new VMD::Motion);
			std::shuffle(KnownMotions.begin(), KnownMotions.end(), RandomGenerator);

			Motion->loadFromFile(KnownMotions.front());

			Motion->attachModel(Model);
		}
		else if (!Paused) {
			if (WaitTime <= 0.0f)
				Motion->advanceFrame(FrameTime * 30.0f);
			else 
				WaitTime -= FrameTime;
		}
	}
#endif

	Model->Update(FrameTime);
}

bool Scenes::Menu::render()
{
	auto Context = Renderer->GetDeviceContext();

	Renderer->SetDepthLessEqual();

	if (!Shader->Update(0, Context))
		return false;
	
	Model->Render(Context, Frustum);

	return true;
}

bool Scenes::Menu::isFinished()
{
	return false;
}

void Scenes::Menu::onAttached()
{
	InputManager->addBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_R), [this](void *unused) {
		if (this->Motion)
			this->Motion->reset();
		this->Model->Reset();
	});
	InputManager->addBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_E), [this](void *unused) {
		this->Paused = false;
	});
	InputManager->addBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_W), [this](void *unused) {
		this->Paused = true;
	});
	InputManager->addBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_X), [this](void *unused) {
		this->Model->GetBoneByName(L"右足ＩＫ")->translate(btVector3(0, 0, -1.0f));
	});
	InputManager->addBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_Z), [this](void *unused) {
		this->Model->GetBoneByName(L"右足ＩＫ")->translate(btVector3(0, 2.5f, 0));
	});
}

void Scenes::Menu::onDeattached()
{
	InputManager->removeBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_R));
	InputManager->removeBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_E));
	InputManager->removeBinding(Input::CallbackInfo(Input::CallbackInfo::OnKeyUp, DIK_W));
}
