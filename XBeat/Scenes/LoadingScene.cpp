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
#include "../PMX/PMXLoader.h"

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
#include <ResourceCache.h>
#include <Scene.h>
#include <StaticSprite2D.h>

#include <Model.h>
#include <AnimatedModel.h>
#include <StaticModel.h>

#include <algorithm>
#include <chrono>
#include <random>
#include <vector>

namespace fs = boost::filesystem;

Scenes::Loading::Loading(Urho3D::Context *Context, std::future<bool> &&Task)
	: Context(Context), LoadingTask(std::move(Task))
{
}

void Scenes::Loading::initialize()
{
	Urho3D::ResourceCache* Cache = Context->GetSubsystem<Urho3D::ResourceCache>();
	Urho3D::Renderer* Renderer = Context->GetSubsystem<Urho3D::Renderer>();
	Urho3D::Graphics* graphics = Context->GetSubsystem<Urho3D::Graphics>();

	Scene = new Urho3D::Scene(Context);
	Scene->CreateComponent<Urho3D::Octree>();
	Scene->CreateComponent<Urho3D::DebugRenderer>();

	Urho3D::Node* planeNode = Scene->CreateChild("Plane");
	planeNode->SetScale(Urho3D::Vector3(100.0f, 1.0f, 100.0f));
	Urho3D::StaticModel* planeObject = planeNode->CreateComponent<Urho3D::StaticModel>();
	planeObject->SetModel(Cache->GetResource<Urho3D::Model>("Models/Plane.mdl"));
	planeObject->SetMaterial(Cache->GetResource<Urho3D::Material>("Materials/StoneTiled.xml"));

	// Create a directional light to the world. Enable cascaded shadows on it
	Urho3D::Node* lightNode = Scene->CreateChild("DirectionalLight");
	lightNode->SetDirection(Urho3D::Vector3(0.6f, -1.0f, 0.8f));
	Urho3D::Light* light = lightNode->CreateComponent<Urho3D::Light>();
	light->SetLightType(Urho3D::LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(Urho3D::BiasParameters(0.00025f, 0.5f));
	// Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
	light->SetShadowCascade(Urho3D::CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

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
	CameraNode->SetPosition(Urho3D::Vector3(0.0f, 2.0f, -20.0f));

	auto Camera = CameraNode->CreateComponent<Urho3D::Camera>();
	Camera->SetFarClip(1000.0f);
#if 0
	Camera->SetOrthographic(true);
	Camera->SetOrthoSize((float)graphics->GetHeight() * Urho3D::PIXEL_SIZE);

	Urho3D::Sprite2D* Sprite = nullptr;
	// Try to load a file as a texture
	while (!AvailableFiles.empty()) {
		Sprite = Cache->GetResource<Urho3D::Sprite2D>(AvailableFiles.back().c_str());

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
#else
	using namespace Urho3D;

	Viewport* viewport = new Viewport(Context, Scene, Camera);
	Renderer->SetViewport(0, viewport);

	Node* ModelNode = Scene->CreateChild("Model");
	ModelNode->SetPosition(Vector3(0, 0, 0));
	ModelNode->SetScale(0.3f);
	AnimatedModel* SModel = ModelNode->CreateComponent<AnimatedModel>();
	PMX::Loader loader;
	try {
		auto file = Cache->GetFile("./OldData/Models/White Rock Shooter/WRS/Tda White Rock Shooter.pmx", false);
		char *data = new char[file->GetSize()];
		file->Read(data, file->GetSize());
		const char *p = data;
		if (!loader.loadFromMemory(SModel, p))
			LOGERROR("Failed to load model");
		delete[] data;
	}
	catch (PMX::Loader::Exception &e)
	{
		LOGERROR(e.what());
	}
	SModel->SetCastShadows(true);
#endif
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
