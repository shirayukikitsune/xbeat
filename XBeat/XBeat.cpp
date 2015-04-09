#include "XBeat.h"
#include "Scenes/LoadingScene.h"

#include <Console.h>
#include <Context.h>
#include <DebugHud.h>
#include <Engine.h>
#include <EngineEvents.h>
#include <FileSystem.h>
#include <Graphics.h>
#include <Input.h>
#include <InputEvents.h>
#include <IOEvents.h>
#include <Log.h>
#include <ProcessUtils.h>
#include <Renderer.h>
#include <ResourceCache.h>
#include <UI.h>
#include <XMLFile.h>
#include <Windows.h>

using namespace Urho3D;

DEFINE_APPLICATION_MAIN(XBeat);

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

	// This is the task that will be executed when the loading screen is being shown.
	std::packaged_task<bool()> WaitTask([this] {
		while (true) { std::this_thread::sleep_for(std::chrono::seconds(5)); }

		return true;
	});

	loadingScene = new Scenes::Loading(context_, WaitTask.get_future());

	loadingScene->initialize();

	std::thread(std::move(WaitTask)).detach();
}

void XBeat::SetWindowParameters()
{
	Graphics* graphics = GetSubsystem<Graphics>();
	graphics->SetWindowTitle("XBeat");
}

void XBeat::SubscribeToEvents()
{
	// Subscribe key down event
	SubscribeToEvent(E_KEYDOWN, HANDLER(XBeat, HandleKeyDown));

	SubscribeToEvent(E_CONSOLECOMMAND, HANDLER(XBeat, HandleConsole));
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
	if (key == KEY_ESC)
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
			if (quality > SHADOWQUALITY_HIGH_24BIT)
				quality = SHADOWQUALITY_LOW_16BIT;
			renderer->SetShadowQuality(quality);
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

	if (formatted.Compare("exit") == 0 || formatted.Compare("quit") == 0) {
		engine_->Exit();
	}
	else {
		Urho3D::Log::WriteRaw("Unrecognized command \"" + input + "\"");
	}
}
