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

#include "../shuffle.h"
#include "../PMX/PMXModel.h"
#include "../VMD/MotionController.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <random>

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
	Urho3D::FileSystem* fs = context_->GetSubsystem<Urho3D::FileSystem>();

	Scene = new Urho3D::Scene(context_);
	Scene->CreateComponent<Urho3D::Octree>();
	Scene->CreateComponent<VMD::MotionController>();
	auto physics = Scene->CreateComponent<Urho3D::PhysicsWorld>();
	auto dr = Scene->CreateComponent<Urho3D::DebugRenderer>();

	// Create a list of motions to be used as idle animations
	fs->ScanDir(KnownMotions, "./Data/Motions/Idle/", ".vmd", Urho3D::SCAN_FILES, false);

	CameraNode = Scene->CreateChild("Camera");
	CameraNode->SetPosition(Urho3D::Vector3(0, 10.0f, -50.0f));
	auto Camera = CameraNode->CreateComponent<Urho3D::Camera>();
    Camera->SetFarClip(300.0f);

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

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = Scene->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

	// Create a directional light to the world. Enable cascaded shadows on it
	Urho3D::Node* lightNode = Scene->CreateChild("DirectionalLight");
	lightNode->SetDirection(Urho3D::Vector3(0.6f, -1.0f, 1.0f));
	Urho3D::Light* light = lightNode->CreateComponent<Urho3D::Light>();
	light->SetLightType(Urho3D::LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(Urho3D::BiasParameters(0.00025f, 0.5f));
	// Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
	light->SetShadowCascade(Urho3D::CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

	/*Urho3D::Node* skyNode = Scene->CreateChild("Sky");
	Urho3D::Skybox* skybox = skyNode->CreateComponent<Urho3D::Skybox>();
	skybox->SetModel(Cache->GetResource<Urho3D::Model>("Models/Box.mdl"));
	skybox->SetMaterial(Cache->GetResource<Urho3D::Material>("Materials/Skybox.xml"));*/

	Viewport* viewport = new Viewport(context_, Scene, Camera);
    viewport->SetRenderPath(Cache->GetResource<XMLFile>("RenderPaths/ForwardHWDepth.xml", false));
    Renderer->SetViewport(0, viewport);

	Node* Model1Node = Scene->CreateChild("Model1");
	Model1Node->SetPosition(Vector3(-8, 0, 0));
    AnimatedModel* SModel1 = Model1Node->CreateComponent<AnimatedModel>();

	/*Node* Model2Node = Scene->CreateChild("Model2");
	Model2Node->SetPosition(Vector3(8, 0, 0));
    AnimatedModel* SModel2 = Model2Node->CreateComponent<AnimatedModel>();*/
	try {
        auto model = Cache->GetResource<PMXModel>("Data/Models/Maika v1.1/MAIKAv1.1.pmx");
        PMXAnimatedModel::SetModel(model, SModel1, "Data/Models/Maika v1.1/");
        SModel1->SetOccludee(false);

        /*model = Cache->GetResource<PMXModel>("Data/Models/new/MikuV4X_Digitrevx/MikuV4X.pmx");
        PMXAnimatedModel::SetModel(model, SModel2, "Data/Models/new/MikuV4X_Digitrevx/");
        SModel2->SetOccludee(false);*/
	}
	catch (PMXModel::Exception &e)
	{
        URHO3D_LOGERROR(e.what());
	}
	SModel1->SetCastShadows(true);
	//SModel2->SetCastShadows(true);

	SubscribeToEvent(Scene, Urho3D::E_SCENEUPDATE, URHO3D_HANDLER(Scenes::Menu, HandleSceneUpdate));
    SubscribeToEvent(Urho3D::E_POSTRENDERUPDATE, URHO3D_HANDLER(Scenes::Menu, HandlePostRenderUpdate));
    SubscribeToEvent(Urho3D::E_CONSOLECOMMAND, URHO3D_HANDLER(Scenes::Menu, HandleConsole));
}

void Scenes::Menu::HandlePostRenderUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    // If draw debug mode is enabled, draw viewport debug geometry, which will show eg. drawable bounding boxes and skeleton
    // bones. Note that debug geometry has to be separately requested each frame. Disable depth test so that we can see the
    // bones properly
    if (debugRender)
        GetSubsystem<Urho3D::Renderer>()->DrawDebugGeometry(false);
}

void Scenes::Menu::HandleConsole(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData)
{
    using namespace Urho3D;
    using namespace Urho3D::ConsoleCommand;

    auto & cmd = eventData[P_COMMAND].GetString();

    if (cmd.SubstringUTF8(0, 8).Compare("mdl_load") == 0) {
        ResourceCache* Cache = context_->GetSubsystem<ResourceCache>();

        auto & mdlFile = cmd.SubstringUTF8(9).Replaced("\\", "/");
        auto node = Scene->CreateChild();
        AnimatedModel* amodel = node->CreateComponent<AnimatedModel>();

        try {
            auto model = Cache->GetResource<PMXModel>(mdlFile);
            PMXAnimatedModel::SetModel(model, amodel, mdlFile.SubstringUTF8(0, mdlFile.FindLast('/') + 1));
            amodel->SetOccludee(false);
            amodel->SetCastShadows(true);
        }
        catch (PMXModel::Exception &e)
        {
            URHO3D_LOGERROR(e.what());
        }
    }
    else if (cmd.SubstringUTF8(0, 8).Compare("scn_save") == 0) {
        auto & filename = cmd.SubstringUTF8(9);

        File f(context_, filename, FILE_WRITE);
        Scene->SaveJSON(f);
        f.Close();
    }
}

void Scenes::Menu::HandleSceneUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    using namespace Urho3D;
	using namespace Urho3D::SceneUpdate;

    float timeStep = eventData[P_TIMESTEP].GetFloat();
    
    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input->GetMouseMove();
    yaw += MOUSE_SENSITIVITY * mouseMove.x_;
    pitch += MOUSE_SENSITIVITY * mouseMove.y_;
    pitch = Clamp(pitch, -90.0f, 90.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    CameraNode->SetRotation(Quaternion(pitch, yaw, 0.0f));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        CameraNode->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        CameraNode->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        CameraNode->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        CameraNode->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    // Toggle debug geometry with space
    if (input->GetKeyPress(KEY_SPACE))
        debugRender = !debugRender;

    // Check if a new motion should be loaded
	if (KnownMotions.Empty())
		return;

	if (!Motion || (Motion->isFinished() && waitTime <= 0.0f)) {
		// Set the model to its initial state
		auto Model = Scene->GetChild("Model1");
		Model->GetComponent<AnimatedModel>()->GetSkeleton().Reset();

		// Add a waiting time 
		if (Motion != nullptr)
			waitTime = (float)RandomGenerator() / (float)RandomGenerator.max() * 15.0f;

		// Shuffle the motion vector
		Urho3D::random_shuffle(KnownMotions.Begin(), KnownMotions.End(), RandomGenerator);

		// Initialize a new random VMD motion
		Motion = Scene->GetComponent<VMD::MotionController>()->LoadMotion("./Data/Motions/Idle/" + KnownMotions.Front());

		if (Motion)
			Motion->attachModel(Model);
	}
	else
		waitTime -= timeStep;
}
