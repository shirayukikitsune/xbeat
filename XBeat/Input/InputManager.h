//===-- Input/InputManager.h - Declares the Manager for Input Devices ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===-------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the InputManager class, which manages
/// all used input devices.
///
//===-------------------------------------------------------------------------------===//

#pragma once

#include <bitset>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>

#define DIRECTINPUT_VERSION 0x0800

#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "xinput.lib")

#include <dinput.h>
#include "../Dispatcher.h"
#include <Xinput.h>

namespace Input {

/// \brief Describes the movement of the mouse
struct MouseMovement
{
	/// \brief Movement in X axis
	long x;
	/// \brief Movement in Y axis
	long y;
};

/// \brief Describes the movement of a XInput axis
struct ThumbMovement
{
	/// \brief Movement in X axis
	float dx;
	/// \brief Movement in Y axis
	float dy;
};

/// \brief Defines the mouse buttons that are supported by the InputManager class
enum struct MouseButton {
	Left,
	Right,
	Middle,
	Button3,
	Button4,
	Button5,
	Button6,
	Button7
};

/// \brief Wrapper for callback informations
struct CallbackInfo
{
	/// \brief Defines which callback event this information is associated with
	enum Event {
		OnKeyUp,
		OnKeyDown,
		OnKeyPressed,
		OnMouseMove,
		OnMouseDown,
		OnMouseUp,
		OnMousePress,
		OnGamepadDown,
		OnGamepadUp,
		OnGamepadPress,
		OnGamepadLeftThumb,
		OnGamepadRightThumb,
		OnGamepadLeftTrigger,
		OnGamepadRightTrigger,
	};

	/// \brief The Event that this information is associated with
	Event EventType;
	/// \brief The button that this callback is associated with
	uint16_t Button;
	/// \brief A parameter to be used when this callback is fired
	void *Parameter;

	CallbackInfo(Event EventType, uint16_t Button = 0, void *Parameter = nullptr) : EventType(EventType), Button(Button), Parameter(Parameter) {};

};

/// \name Functions to help ordering of CallbackInfo 
/// @{
bool operator< (const CallbackInfo& One, const CallbackInfo& Other);
bool operator== (const CallbackInfo& One, const CallbackInfo& Other);
/// @}

/// \brief The function type of a CallbackInfo
typedef std::function<void(void*)> Callback;
/// \brief The function type of a MouseMovement
typedef std::function<void(std::shared_ptr<MouseMovement>)> MouseMoveCallback;

/// \brief Handles all supported input devices
class Manager
{
public:
	enum {
		/// \brief How many keys are processed per frame
		KeyboardBufferSize = 32
	};

	Manager();
	~Manager();

	/// \brief Initializes the input manager, instanciating all interfaces
	bool initialize(HINSTANCE Instance, HWND Window, std::shared_ptr<Dispatcher> EventDispatcher);
	/// \brief Stops processing input, deleting all interface references
	void shutdown();
	/// \brief Processes the state of input devices at an instant of time
	bool runFrame();

	/// \brief Checks if a keyboard's key is pressed
	bool isKeyPressed(int Key);

	/// \brief Adds a binding to be fired
	void addBinding(CallbackInfo &Information, Callback Binding);
	/// \brief Removes a callback from the list of bindings
	void removeBinding(CallbackInfo &Information);

	/// \brief Defines the callback to be fired when the mouse is moved
	void setMouseBinding(MouseMoveCallback Binding);
	/// \brief Removes the callback to be fired when the mouse is moved
	void removeMouseBinding();

private:
	/// \brief Reads the state of the keyboard
	bool readKeyboard();
	/// \brief Reads the state of the mouse
	bool readMouse();
	/// \brief Does all the input processing, firing the bindings if needed
	void processInput();

	/// \brief Converts raw stick values to relative values (0.0f to 1.0f of an axis)
	ThumbMovement* normalizeStickValues(float ValueX, float ValueY, int Deadzone);

	/// \brief An DirectInput interface instance
	IDirectInput8 *DirectInputInterface;
	/// \brief The DirectInput keyboard device instance
	IDirectInputDevice8 *KeyboardInterface;
	/// \brief The DirectInput mouse device instance
	IDirectInputDevice8 *MouseInterface;

	/// \brief The state of the keyboard
	uint8_t KeyState[256];
	/// \brief The keys that are currently pressed
	std::unordered_map<uint8_t, CallbackInfo::Event> PressedKeys;
	/// \brief The current and last mouse states
	DIMOUSESTATE2 CurrentMouseState, LastMouseState;
	/// \brief The last XInput device state
	///
	/// \remarks This will be zeroed if the XInput device is lost
	XINPUT_STATE LastXInputState;

	/// \brief The callback to be fired when the mouse is moved
	MouseMoveCallback MouseCallback;
	/// \brief All callbacks registered
	std::map<CallbackInfo, Callback> Bindings;
	/// \brief The class that will run the events
	std::shared_ptr<Dispatcher> EventDispatcher;
};

}
