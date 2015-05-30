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
#include "../PMX/PMXModel.h"

#include <AnimatedModel.h>
#include <Camera.h>
#include <CollisionShape.h>
#include <Context.h>
#include <DebugRenderer.h>
#include <FileSystem.h>
#include <Graphics.h>
#include <Light.h>
#include <Log.h>
#include <Material.h>
#include <Model.h>
#include <Octree.h>
#include <Renderer.h>
#include <RenderPath.h>
#include <ResourceCache.h>
#include <RigidBody.h>
#include <Scene.h>
#include <Skybox.h>
#include <StaticModel.h>
#include <XMLFile.h>

#include <boost/filesystem.hpp>
#include <random>

namespace fs = boost::filesystem;

Scenes::Menu::Menu(Urho3D::Context *Context)
	: Context(Context)
{
	std::random_device RandomDevice;
	RandomGenerator.seed(RandomDevice());
}

Scenes::Menu::~Menu()
{
}

void Scenes::Menu::initialize()
{
	Urho3D::ResourceCache* Cache = Context->GetSubsystem<Urho3D::ResourceCache>();
	Urho3D::Renderer* Renderer = Context->GetSubsystem<Urho3D::Renderer>();
	Urho3D::Graphics* graphics = Context->GetSubsystem<Urho3D::Graphics>();

	Scene = new Urho3D::Scene(Context);
	Scene->CreateComponent<Urho3D::Octree>();
	auto dr = Scene->CreateComponent<Urho3D::DebugRenderer>();

#if 0
	// Create a list of motions to be used as idle animations
	fs::directory_iterator EndIterator;
	fs::path MotionPath(L"./Data/Motions/Idle/");
	for (fs::directory_iterator Entry(MotionPath); Entry != EndIterator; Entry++) {
		if (fs::is_regular_file(Entry->status()) && Entry->path().extension().wstring().compare(L".vmd") == 0) {
			KnownMotions.Push(Entry->path().generic_wstring().c_str());
		}
	}
#endif

	CameraNode = Scene->CreateChild("Camera");
	CameraNode->SetPosition(Urho3D::Vector3(0, 10.0f, -30.0f));
	auto Camera = CameraNode->CreateComponent<Urho3D::Camera>();

	using namespace Urho3D;

	Urho3D::Node* planeNode = Scene->CreateChild("Plane");
	planeNode->SetScale(Urho3D::Vector3(100.0f, 1.0f, 100.0f));
	Urho3D::StaticModel* planeObject = planeNode->CreateComponent<Urho3D::StaticModel>();
	planeObject->SetModel(Cache->GetResource<Urho3D::Model>("Models/Plane.mdl"));
	planeObject->SetMaterial(Cache->GetResource<Urho3D::Material>("Materials/StoneTiled.xml"));
	Urho3D::CollisionShape* collisionShape = planeNode->CreateComponent<Urho3D::CollisionShape>();
	collisionShape->SetShapeType(Urho3D::SHAPE_STATICPLANE);
	collisionShape->SetStaticPlane();
	Urho3D::RigidBody* rigidBody = planeNode->CreateComponent<Urho3D::RigidBody>();
	rigidBody->SetKinematic(true);
	rigidBody->SetUseGravity(false);

	// Create a directional light to the world. Enable cascaded shadows on it
	Urho3D::Node* lightNode = Scene->CreateChild("DirectionalLight");
	lightNode->SetDirection(Urho3D::Vector3(0.6f, -1.0f, 0.8f));
	Urho3D::Light* light = lightNode->CreateComponent<Urho3D::Light>();
	light->SetLightType(Urho3D::LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(Urho3D::BiasParameters(0.00025f, 0.5f));
	// Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
	light->SetShadowCascade(Urho3D::CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
	light->SetSpecularIntensity(0.5f);
	light->SetColor(Urho3D::Color(1.2f, 1.2f, 1.2f));

	Urho3D::Node* skyNode = Scene->CreateChild("Sky");
	skyNode->SetScale(500.0f); // The scale actually does not matter
	Urho3D::Skybox* skybox = skyNode->CreateComponent<Urho3D::Skybox>();
	skybox->SetModel(Cache->GetResource<Urho3D::Model>("Models/Box.mdl"));
	skybox->SetMaterial(Cache->GetResource<Urho3D::Material>("Materials/Skybox.xml"));

	Viewport* viewport = new Viewport(Context, Scene, Camera);
	Renderer->SetViewport(0, viewport);

	auto file = Cache->GetFile("RenderPaths/PrepassHDR.xml", false);
	char *data = new char[file->GetSize()];
	file->Read(data, file->GetSize());
	XMLFile* xmlfile = new XMLFile(Context);
	xmlfile->FromString(data);
	Renderer->SetDefaultRenderPath(xmlfile);
	delete[] data;

	Node* ModelNode = Scene->CreateChild("Model");
	ModelNode->SetPosition(Vector3(0, 0, 0));
	PMXAnimatedModel* SModel = ModelNode->CreateComponent<PMXAnimatedModel>();
	try {
		auto fs = Scene->GetSubsystem<FileSystem>();
		auto dir = fs->GetCurrentDir();
		fs->SetCurrentDir("OldData/Models/2013RacingMikuMMD");
		auto model = Cache->GetResource<PMXModel>("2013RacingMiku.pmx", false);
		SModel->SetModel(model);
		fs->SetCurrentDir(dir);
	}
	catch (PMXModel::Exception &e)
	{
		LOGERROR(e.what());
	}
	dr->AddSkeleton(SModel->GetSkeleton(), Color::RED);
	SModel->SetCastShadows(true);
}

/*
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
*/