//===-- Scenes/LoadingScene.cpp - Defines the class for the loading scene ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===-----------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines everything related to the loading scene class 
///
//===-----------------------------------------------------------------------------------===//

#include "LoadingScene.h"

#include "../Renderer/Shaders/Texture.h"
#include "../Renderer/D3DRenderer.h"
#include "../Renderer/OrthoWindowClass.h"
#include "../Renderer/Texture.h"

#include <boost/filesystem.hpp>
#include <algorithm>
#include <chrono>
#include <random>
#include <vector>

namespace fs = boost::filesystem;

Scenes::Loading::Loading(std::future<void> &&Task)
	: LoadingTask(std::move(Task))
{
}

Scenes::Loading::~Loading()
{
}

bool Scenes::Loading::initialize()
{
	Window.reset(new Renderer::OrthoWindowClass);
	assert(Window);
	auto &Viewport = Renderer->getViewport();
	if (!Window->Initialize(Renderer->GetDevice(), (int)Viewport.Width, (int)Viewport.Height))
		return false;

	TextureShader.reset(new Renderer::Shaders::Texture);
	assert(TextureShader);
	if (!TextureShader->Initialize(Renderer->GetDevice(), nullptr))
		return false;

	// Select a random texture file
	fs::directory_iterator EndIterator;
	std::vector<std::wstring> AvailableFiles;
	for (fs::directory_iterator PathIterator(fs::path(L"./Data/Textures/Loading/")); PathIterator != EndIterator; ++PathIterator) {
		if (fs::is_regular_file(PathIterator->status())) {
			AvailableFiles.emplace_back(PathIterator->path().generic_wstring());
		}
	}

	// Shuffle the available file list
	std::random_device RandomDevice;
	std::mt19937 RandomGenerator(RandomDevice());
	std::shuffle(AvailableFiles.begin(), AvailableFiles.end(), RandomGenerator);

	// Try to load a file as a texture
	while (!AvailableFiles.empty()) {
		Texture.reset(new Renderer::Texture);
		assert(Texture);
		if (!Texture->Initialize(Renderer->GetDevice(), AvailableFiles.back())) {
			AvailableFiles.pop_back();
			continue;
		}

		// Texture successfully loaded, break out of the loop
		break;
	}

	return true;
}

void Scenes::Loading::shutdown()
{
	Window.reset();
	Texture.reset();
	TextureShader.reset();
}

void Scenes::Loading::frame(float FrameTime)
{
	// TODO: What can be done here:
	//  - "Now Loading" animations
	//  - Minigames? :P
}

bool Scenes::Loading::render()
{
	auto Context = Renderer->GetDeviceContext();

	Renderer->Begin2D();

	Window->Render(Context);

	auto ReturnValue = TextureShader->Render(Context, Window->GetIndexCount(), Texture->GetTexture());

	Renderer->End2D();

	return ReturnValue;
}

bool Scenes::Loading::isFinished()
{
	// The LoadingScene will exit if there is nothing to load!
	if (!LoadingTask.valid())
		return true;

	// Retrieve the async task status
	auto status = LoadingTask.wait_for(std::chrono::seconds(0));
	return status == std::future_status::ready;
}
