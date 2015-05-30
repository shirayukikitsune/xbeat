#pragma once

#include "Scenes/LoadingScene.h"
#include "Scenes/MenuScene.h"

#include <Application.h>

class XBeat : public Urho3D::Application
{
	OBJECT(XBeat);

public:
	XBeat(Urho3D::Context *Context);

	/// Setup before engine initialization. Modifies the engine parameters.
	virtual void Setup();
	/// Setup after engine initialization. Creates the logo, console & debug HUD.
	virtual void Start();
	/// Cleanup after the main loop. Called by Application.
	//virtual void Stop();

protected:
	void SetWindowParameters();

	void CreateConsoleAndDebugHud();

	void HandleKeyDown(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
	void HandleConsole(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
	void HandleInput(const Urho3D::String& string);

	/// Subscribe to application-wide logic update and post-render update events.
	void SubscribeToEvents();

	/// Flag to indicate whether touch input has been enabled.
	bool touchEnabled_;
	/// Screen joystick index for navigational controls (mobile platforms only).
	unsigned screenJoystickIndex_;
	/// Screen joystick index for settings (mobile platforms only).
	unsigned screenJoystickSettingsIndex_;
	/// Pause flag.
	bool paused_;

	Scenes::LoadingScene *loadingScene;
	Scenes::Menu *menuScene;
};

