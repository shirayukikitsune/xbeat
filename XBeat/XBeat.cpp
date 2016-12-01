#include "XBeat.h"
#include "Scenes/LoadingScene.h"
#include "PMX/PMXIKNode.h"
#include "PMX/PMXIKSolver.h"
#include "PMX/PMXModel.h"
#include "PMX/PMXRigidBody.h"
#include "VMD/MotionController.h"

#include <Urho3D/Engine/Console.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/UI.h>

#include <Windows.h>

using namespace Urho3D;

URHO3D_DEFINE_APPLICATION_MAIN(XBeat);

XBeat::XBeat(Context* Context)
	: Application(Context)
{

}

void XBeat::Setup()
{
	// Modify engine startup parameters
	engineParameters_["WindowTitle"] = GetTypeName();
	engineParameters_["LogName"] = GetSubsystem<FileSystem>()->GetAppPreferencesDir("urho3d", "logs") + GetTypeName() + ".log";
	engineParameters_["FullScreen"] = false;
	engineParameters_["Headless"] = false;
}

void XBeat::Start()
{
	// Set custom window Title & Icon
	SetWindowParameters();

	// Create console and debug HUD
	CreateConsoleAndDebugHud();

	SubscribeToEvents();

	PMXIKNode::RegisterObject(context_);
	PMXIKSolver::RegisterObject(context_);
	PMXModel::RegisterObject(context_);
	PMXRigidBody::RegisterObject(context_);
	VMD::MotionController::RegisterObject(context_);
	VMD::Motion::RegisterObject(context_);

	//loadingScene = new Scenes::LoadingScene(context_, nullptr);
	//loadingScene->initialize();

	menuScene = new Scenes::Menu(context_);
	menuScene->initialize();
}

void XBeat::Stop()
{
	engine_->DumpResources(true);
}

void XBeat::SetWindowParameters()
{
	Graphics* graphics = GetSubsystem<Graphics>();
	graphics->SetWindowTitle("XBeat");
}

void XBeat::SubscribeToEvents()
{
	// Subscribe key down event
	SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(XBeat, HandleKeyDown));

	SubscribeToEvent(E_CONSOLECOMMAND, URHO3D_HANDLER(XBeat, HandleConsole));
}

void XBeat::CreateConsoleAndDebugHud()
{
	// Get default style
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	XMLFile* xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

	// Create console
	Console* console = engine_->CreateConsole();
	console->SetDefaultStyle(xmlFile);
	console->GetBackground()->SetOpacity(0.8f);
	console->SetCommandInterpreter(GetTypeName());

	// Create debug HUD.
	DebugHud* debugHud = engine_->CreateDebugHud();
	debugHud->SetDefaultStyle(xmlFile);
}

void XBeat::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;

	int key = eventData[P_KEY].GetInt();

	// Close console (if open) or exit when ESC is pressed
	if (key == KEY_ESCAPE)
	{
		Console* console = GetSubsystem<Console>();
		if (console->IsVisible())
			console->SetVisible(false);
		else
			engine_->Exit();
	}

	// Toggle console with F1
	else if (key == KEY_F1)
		GetSubsystem<Console>()->Toggle();

	// Toggle debug HUD with F2
	else if (key == KEY_F2)
		GetSubsystem<DebugHud>()->ToggleAll();

	// Common rendering quality controls, only when UI has no focused element
	else if (!GetSubsystem<UI>()->GetFocusElement())
	{
		Renderer* renderer = GetSubsystem<Renderer>();

		// Preferences / Pause
		if (key == KEY_SELECT && touchEnabled_)
		{
			paused_ = !paused_;

			Input* input = GetSubsystem<Input>();
			if (screenJoystickSettingsIndex_ == M_MAX_UNSIGNED)
			{
				// Lazy initialization
				ResourceCache* cache = GetSubsystem<ResourceCache>();
				screenJoystickSettingsIndex_ = input->AddScreenJoystick(cache->GetResource<XMLFile>("UI/ScreenJoystickSettings_Samples.xml"), cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
			}
			else
				input->SetScreenJoystickVisible(screenJoystickSettingsIndex_, paused_);
		}

		// Texture quality
		else if (key == '1')
		{
			int quality = renderer->GetTextureQuality();
			++quality;
			if (quality > QUALITY_HIGH)
				quality = QUALITY_LOW;
			renderer->SetTextureQuality(quality);
		}

		// Material quality
		else if (key == '2')
		{
			int quality = renderer->GetMaterialQuality();
			++quality;
			if (quality > QUALITY_HIGH)
				quality = QUALITY_LOW;
			renderer->SetMaterialQuality(quality);
		}

		// Specular lighting
		else if (key == '3')
			renderer->SetSpecularLighting(!renderer->GetSpecularLighting());

		// Shadow rendering
		else if (key == '4')
			renderer->SetDrawShadows(!renderer->GetDrawShadows());

		// Shadow map resolution
		else if (key == '5')
		{
			int shadowMapSize = renderer->GetShadowMapSize();
			shadowMapSize *= 2;
			if (shadowMapSize > 2048)
				shadowMapSize = 512;
			renderer->SetShadowMapSize(shadowMapSize);
		}

		// Shadow depth and filtering quality
		else if (key == '6')
		{
			int quality = renderer->GetShadowQuality();
			++quality;
			if (quality > SHADOWQUALITY_BLUR_VSM)
				quality = SHADOWQUALITY_SIMPLE_16BIT;
			renderer->SetShadowQuality((ShadowQuality)quality);
		}

		// Occlusion culling
		else if (key == '7')
		{
			bool occlusion = renderer->GetMaxOccluderTriangles() > 0;
			occlusion = !occlusion;
			renderer->SetMaxOccluderTriangles(occlusion ? 5000 : 0);
		}

		// Instancing
		else if (key == '8')
			renderer->SetDynamicInstancing(!renderer->GetDynamicInstancing());

		// Take screenshot
		else if (key == '9')
		{
			Graphics* graphics = GetSubsystem<Graphics>();
			Image screenshot(context_);
			graphics->TakeScreenShot(screenshot);
			// Here we save in the Data folder with date and time appended
			screenshot.SavePNG(GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
				Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
		}
	}
}

void XBeat::HandleConsole(StringHash eventType, VariantMap& eventData)
{
	using namespace ConsoleCommand;
	if (eventData[P_ID].GetString() == GetTypeName())
		HandleInput(eventData[P_COMMAND].GetString());
}

void XBeat::HandleInput(const String& input)
{
	Console* console = GetSubsystem<Console>();
	String formatted = input.ToLower().Trimmed();
	if (formatted.Empty()) {
		return;
	}

	if (formatted.Compare("exit") == 0 || formatted.Compare("quit") == 0 || formatted.Compare("qqq") == 0) {
		engine_->Exit();
	}
	else {
		Urho3D::Log::WriteRaw("Unrecognized command \"" + input + "\"", true);
	}
}
