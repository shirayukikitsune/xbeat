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

#include "../shuffle.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/UI.h>

#include <algorithm>
#include <chrono>
#include <random>
#include <vector>

Scenes::LoadingScene::LoadingScene(Urho3D::Context *Context, Urho3D::Scene *NextScene)
	: Context(Context), NextScene(NextScene)
{
}

void Scenes::LoadingScene::initialize()
{
	Urho3D::ResourceCache* Cache = Context->GetSubsystem<Urho3D::ResourceCache>();
	Urho3D::Graphics* Graphics = Context->GetSubsystem<Urho3D::Graphics>();
	Urho3D::FileSystem* FS = Context->GetSubsystem<Urho3D::FileSystem>();
	Urho3D::UI* UI = Context->GetSubsystem<Urho3D::UI>();

	// Select a random texture file
	static const Urho3D::String LoadingTexPath("./Data/Textures/Loading/");
	Urho3D::Vector<Urho3D::String> AvailableFiles;
	FS->ScanDir(AvailableFiles, LoadingTexPath, "", Urho3D::SCAN_FILES, false);

	// Shuffle the available file list
	std::random_device RandomDevice;
	std::mt19937 RandomGenerator(RandomDevice());
	Urho3D::random_shuffle(AvailableFiles.Begin(), AvailableFiles.End(), RandomGenerator);

	Urho3D::Texture2D* SpriteTex = nullptr;
	// Try to load a file as a texture
	while (!AvailableFiles.Empty()) {
		SpriteTex = Cache->GetResource<Urho3D::Texture2D>(LoadingTexPath + AvailableFiles.Back(), false);

		if (!SpriteTex) {
			AvailableFiles.Pop();
			continue;
		}

		// Texture successfully loaded, break out of the loop
		break;
	}

	int width = Graphics->GetWidth();
	int height = Graphics->GetHeight();

	if (SpriteTex != nullptr) {
		Urho3D::SharedPtr<Urho3D::Sprite> Sprite(new Urho3D::Sprite(Context));

		Sprite->SetTexture(SpriteTex);
		Sprite->SetPosition(0.0f, 0.0f);
		Sprite->SetHotSpot(0, 0);
		Sprite->SetSize(width, height);
		Sprite->SetBlendMode(Urho3D::BLEND_REPLACE);

		UI->GetRoot()->AddChild(Sprite);
	}
}

bool Scenes::LoadingScene::isFinished()
{
	return NextScene.Null() || NextScene->GetAsyncProgress() >= 1.0f;
}
