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

#include <boost/filesystem.hpp>
#include <Camera.h>
#include <Context.h>
#include <DebugRenderer.h>
#include <Drawable2D.h>
#include <Graphics.h>
#include <Log.h>
#include <Node.h>
#include <Octree.h>
#include <Renderer.h>
#include <Resource.h>
#include <ResourceCache.h>
#include <Scene.h>
#include <StaticSprite2D.h>

#include <algorithm>
#include <chrono>
#include <random>
#include <vector>

namespace fs = boost::filesystem;

Scenes::LoadingScene::LoadingScene(Urho3D::Context *Context, Urho3D::Scene *NextScene)
	: Context(Context), NextScene(NextScene)
{
}

void Scenes::LoadingScene::initialize()
{
	Urho3D::ResourceCache* Cache = Context->GetSubsystem<Urho3D::ResourceCache>();
	Urho3D::Renderer* Renderer = Context->GetSubsystem<Urho3D::Renderer>();
	Urho3D::Graphics* graphics = Context->GetSubsystem<Urho3D::Graphics>();

	Scene = new Urho3D::Scene(Context);
	Scene->CreateComponent<Urho3D::Octree>();
	auto dr = Scene->CreateComponent<Urho3D::DebugRenderer>();

	// Select a random texture file
	fs::directory_iterator EndIterator;
	std::vector<std::string> AvailableFiles;
	for (fs::directory_iterator PathIterator(fs::path(L"./Data/Textures/Loading/")); PathIterator != EndIterator; ++PathIterator) {
		if (fs::is_regular_file(PathIterator->status())) {
			AvailableFiles.emplace_back(PathIterator->path().generic_string());
		}
	}

	// Shuffle the available file list
	std::random_device RandomDevice;
	std::mt19937 RandomGenerator(RandomDevice());
	std::shuffle(AvailableFiles.begin(), AvailableFiles.end(), RandomGenerator);

	CameraNode = Scene->CreateChild("Camera");
	CameraNode->SetPosition(Urho3D::Vector3::ZERO);

	auto Camera = CameraNode->CreateComponent<Urho3D::Camera>();
	Camera->SetOrthographic(true);
	Camera->SetOrthoSize((float)graphics->GetHeight() * Urho3D::PIXEL_SIZE);

	Urho3D::Sprite2D* Sprite = nullptr;
	// Try to load a file as a texture
	while (!AvailableFiles.empty()) {
		Sprite = Cache->GetResource<Urho3D::Sprite2D>(AvailableFiles.back().c_str(), false);

		if (!Sprite) {
			AvailableFiles.pop_back();
			continue;
		}

		// Texture successfully loaded, break out of the loop
		break;
	}

	float halfWidth = graphics->GetWidth() * 0.5f * Urho3D::PIXEL_SIZE;
	float halfHeight = graphics->GetHeight() * 0.5f * Urho3D::PIXEL_SIZE;

	if (Sprite != nullptr) {
		auto Size = Sprite->GetRectangle();
		Camera->SetOrthoSize(Urho3D::Vector2((float)(Size.right_ - Size.left_) * Urho3D::PIXEL_SIZE, (float)(Size.bottom_ - Size.top_) * Urho3D::PIXEL_SIZE));

		Urho3D::SharedPtr<Urho3D::Node> SpriteNode(Scene->CreateChild("Background"));
		SpriteNode->SetPosition2D(0.0f, 0.0f);

		Urho3D::StaticSprite2D* StaticSprite = SpriteNode->CreateComponent<Urho3D::StaticSprite2D>();
		StaticSprite->SetSprite(Sprite);

		Urho3D::SharedPtr<Urho3D::Viewport> Viewport(new Urho3D::Viewport(Context, Scene, Camera));
		Renderer->SetViewport(0, Viewport);
	}
}

bool Scenes::LoadingScene::isFinished()
{
	return NextScene.Null() || NextScene->GetAsyncProgress() >= 1.0f;
}
