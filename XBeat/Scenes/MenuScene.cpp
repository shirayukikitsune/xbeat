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
#include "../VMD/MotionController.h"

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
#include <PhysicsWorld.h>
#include <Renderer.h>
#include <RenderPath.h>
#include <ResourceCache.h>
#include <RigidBody.h>
#include <Scene.h>
#include <SceneEvents.h>
#include <Skybox.h>
#include <StaticModel.h>
#include <Swap.h>
#include <XMLFile.h>

#include <boost/filesystem.hpp>
#include <random>

namespace fs = boost::filesystem;

Scenes::Menu::Menu(Urho3D::Context *Context)
	: Urho3D::Object(Context)
{
	std::random_device RandomDevice;
	RandomGenerator.seed(RandomDevice());
}

Scenes::Menu::~Menu()
{
}

void Scenes::Menu::initialize()
{
	Urho3D::ResourceCache* Cache = context_->GetSubsystem<Urho3D::ResourceCache>();
	Urho3D::Renderer* Renderer = context_->GetSubsystem<Urho3D::Renderer>();
	Urho3D::Graphics* graphics = context_->GetSubsystem<Urho3D::Graphics>();

	Scene = new Urho3D::Scene(context_);
	Scene->CreateComponent<Urho3D::Octree>();
	Scene->CreateComponent<VMD::MotionController>();
	auto physics = Scene->CreateComponent<Urho3D::PhysicsWorld>();
	auto dr = Scene->CreateComponent<Urho3D::DebugRenderer>();

	// Create a list of motions to be used as idle animations
	fs::directory_iterator EndIterator;
	fs::path MotionPath(L"./Data/Motions/Idle/");
	for (fs::directory_iterator Entry(MotionPath); Entry != EndIterator; Entry++) {
		if (fs::is_regular_file(Entry->status()) && Entry->path().extension().wstring().compare(L".vmd") == 0) {
			KnownMotions.Push(Entry->path().generic_wstring().c_str());
		}
	}

	CameraNode = Scene->CreateChild("Camera");
	CameraNode->SetPosition(Urho3D::Vector3(0, 10.0f, -50.0f));
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
	lightNode->SetDirection(Urho3D::Vector3(0.6f, 0.0f, 1.0f));
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

	Viewport* viewport = new Viewport(context_, Scene, Camera);
	Renderer->SetViewport(0, viewport);

	auto file = Cache->GetFile("RenderPaths/PrepassHDR.xml", false);
	char *data = new char[file->GetSize()];
	file->Read(data, file->GetSize());
	XMLFile* xmlfile = new XMLFile(context_);
	xmlfile->FromString(data);
	Renderer->SetDefaultRenderPath(xmlfile);
	delete[] data;

	Node* ModelNode = Scene->CreateChild("Model");
	ModelNode->SetPosition(Vector3(0, 0, 0));
	PMXAnimatedModel* SModel = ModelNode->CreateComponent<PMXAnimatedModel>();
	try {
		auto fs = Scene->GetSubsystem<FileSystem>();
		auto dir = fs->GetCurrentDir();
		//fs->SetCurrentDir("Data/Models/Maika v1.1");
		//auto model = Cache->GetResource<PMXModel>("MAIKAv1.1.pmx", false);
		fs->SetCurrentDir(L"Data/Models/キャス狐");
		auto model = Cache->GetResource<PMXModel>(L"キャス狐1.02.pmx", false);
		SModel->SetModel(model);
		fs->SetCurrentDir(dir);
	}
	catch (PMXModel::Exception &e)
	{
		LOGERROR(e.what());
	}
	dr->AddSkeleton(SModel->GetSkeleton(), Color::RED);
	SModel->SetCastShadows(true);

	SubscribeToEvent(Scene, Urho3D::E_SCENEUPDATE, HANDLER(Scenes::Menu, HandleSceneUpdate));
}

namespace Urho3D {
	template <class RandomIt, class UniformRandomNumberGenerator>
	void random_shuffle(RandomIt first, RandomIt last, UniformRandomNumberGenerator&& g)
	{
		typedef typename std::uniform_int_distribution<int> distr_t;
		typedef typename distr_t::param_type param_t;

		distr_t D;
		int i, n;
		n = last - first;
		for (i = n - 1; i > 0; --i) {
			Urho3D::Swap(first + i, first + D(g, param_t(0, i)));
		}
	}
}

void Scenes::Menu::HandleSceneUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
	using namespace Urho3D::SceneUpdate;
	// Check if a new motion should be loaded
	if (KnownMotions.Empty())
		return;

	if (!Motion || (Motion->isFinished() && waitTime <= 0.0f)) {
		// Set the model to its initial state
		auto Model = Scene->GetChild("Model");
		Model->GetComponent<PMXAnimatedModel>()->GetSkeleton().Reset();

		// Add a waiting time 
		if (Motion != nullptr)
			waitTime = (float)RandomGenerator() / (float)RandomGenerator.max() * 15.0f;

		// Shuffle the motion vector
		Urho3D::random_shuffle(KnownMotions.Begin(), KnownMotions.End(), RandomGenerator);

		// Initialize a new random VMD motion
		Motion = Scene->GetComponent<VMD::MotionController>()->LoadMotion(KnownMotions.Front());

		Motion->attachModel(Model);
	}
	else
		waitTime -= eventData[P_TIMESTEP].GetFloat();
}
