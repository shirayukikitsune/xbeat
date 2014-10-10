//===-- Input/InputManager.cpp - Defines the Manager for Input Devices ---*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===-------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines everything related to the Input::Manager class, which
/// manages all used input devices.
///
//===-------------------------------------------------------------------------------===//

#include "InputManager.h"

#include <cassert>
#include <queue>

bool Input::operator< (const CallbackInfo& One, const CallbackInfo& Other) {
	if (One.EventType < Other.EventType) return true;
	if (One.EventType == Other.EventType && One.Button < Other.Button) return true;

	return false;
}
bool Input::operator== (const CallbackInfo& One, const CallbackInfo& Other) {
	return One.EventType == Other.EventType && One.Button == Other.Button;
}

Input::Manager::Manager(void)
{
	DirectInputInterface = nullptr;
	KeyboardInterface = nullptr;
	MouseInterface = nullptr;
}


Input::Manager::~Manager(void)
{
	shutdown();
}


bool Input::Manager::initialize(HINSTANCE Instance, HWND Window, std::weak_ptr<Dispatcher> EventDispatcher)
{
	HRESULT Result;

	this->EventDispatcher = EventDispatcher;

	memset(KeyState, 0, sizeof(KeyState));

	Result = DirectInput8Create(Instance, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&DirectInputInterface, NULL);
	if (FAILED(Result))
		return false;

	Result = DirectInputInterface->CreateDevice(GUID_SysKeyboard, &KeyboardInterface, NULL);
	if (FAILED(Result))
		return false;

	Result = KeyboardInterface->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(Result))
		return false;

	Result = KeyboardInterface->SetCooperativeLevel(Window, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (FAILED(Result))
		return false; 

	DIPROPDWORD prop;
	prop.diph.dwSize = sizeof(DIPROPDWORD);
	prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	prop.diph.dwObj = 0;
	prop.diph.dwHow = DIPH_DEVICE;
	prop.dwData = KeyboardBufferSize; 

	Result = KeyboardInterface->SetProperty(DIPROP_BUFFERSIZE, &prop.diph);
	if (FAILED(Result))
		return false;

	KeyboardInterface->Acquire();

	Result = DirectInputInterface->CreateDevice(GUID_SysMouse, &MouseInterface, NULL);
	if (FAILED(Result))
		return false;

	Result = MouseInterface->SetDataFormat(&c_dfDIMouse2);
	if (FAILED(Result))
		return false;

	Result = MouseInterface->SetCooperativeLevel(Window, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (FAILED(Result))
		return false;

	MouseInterface->Acquire();

	return true;
}

void Input::Manager::shutdown()
{
	if (MouseInterface) {
		MouseInterface->Unacquire();
		MouseInterface->Release();
		MouseInterface = nullptr;
	}

	if (KeyboardInterface) {
		KeyboardInterface->Unacquire();
		KeyboardInterface->Release();
		KeyboardInterface = nullptr;
	}

	if (DirectInputInterface) {
		DirectInputInterface->Release();
		DirectInputInterface = nullptr;
	}
}

bool Input::Manager::doFrame()
{
	if (!readKeyboard())
		return false;

	if (!readMouse())
		return false;

	processInput();
	return true;
}

bool Input::Manager::readKeyboard()
{
	HRESULT Result;

	Result = KeyboardInterface->GetDeviceState(sizeof(KeyState), (LPVOID)&KeyState);
	if (FAILED(Result)) {
		if (Result == DIERR_INPUTLOST || Result == DIERR_NOTACQUIRED) {
			KeyboardInterface->Acquire();
		}
		else
			return false;
	}

	return true;
}

bool Input::Manager::readMouse()
{
	HRESULT Result;

	Result = MouseInterface->GetDeviceState(sizeof(DIMOUSESTATE2), (LPVOID)&CurrentMouseState);
	if (FAILED(Result)) {
		if (Result == DIERR_INPUTLOST || Result == DIERR_NOTACQUIRED)
			MouseInterface->Acquire();
		else
			return false;
	}

	return true;
}

void Input::Manager::processInput()
{
	DIDEVICEOBJECTDATA KeyChanges[KeyboardBufferSize];
	DWORD KeyChangeCount = KeyboardBufferSize;
	XINPUT_STATE CurrentXInputState;
	HRESULT Result;
	Result = KeyboardInterface->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), KeyChanges, &KeyChangeCount, 0);
	auto BindingsEnd = Bindings.end(), Binding = Bindings.end();
	std::queue<uint8_t> KeyUpEvents;

	std::shared_ptr<Dispatcher> EventDispatcher = this->EventDispatcher.lock();
	assert(EventDispatcher);

	if (!FAILED(Result)) {
		for (DWORD i = 0; i < KeyChangeCount; i++) {
			PressedKeys[(uint8_t)KeyChanges[i].dwOfs] = (KeyChanges[i].dwData & 0x80) != 0 ? CallbackInfo::OnKeyDown : CallbackInfo::OnKeyUp;
		}
	}

	for (auto &Key : PressedKeys) {
		Binding = Bindings.find(CallbackInfo(Key.second, Key.first));
		if (Binding != BindingsEnd)
			EventDispatcher->addTask(std::bind(Binding->second, Binding->first.Parameter));
		if (Key.second == CallbackInfo::OnKeyDown)
			Key.second = CallbackInfo::OnKeyPressed;
		else if (Key.second == CallbackInfo::OnKeyUp) KeyUpEvents.push(Key.first);
	}

	while (!KeyUpEvents.empty()) {
		PressedKeys.erase(KeyUpEvents.front());
		KeyUpEvents.pop();
	}

	// Check for mouse movements
	if (MouseCallback) {
		std::shared_ptr<MouseMovement> MovementInfo(new MouseMovement);
		MovementInfo->x = CurrentMouseState.lX;
		MovementInfo->y = CurrentMouseState.lY;
		EventDispatcher->addTask(std::bind(MouseCallback, MovementInfo));
	}
	for (int i = 0; i < 8; i++)
	{
		if (LastMouseState.rgbButtons[i] != CurrentMouseState.rgbButtons[i]) {
			if (CurrentMouseState.rgbButtons[i] & 0x80) // Was released, now is pressed
				Binding = Bindings.find(CallbackInfo(CallbackInfo::OnMouseDown, i));
			else Binding = Bindings.find(CallbackInfo(CallbackInfo::OnMouseUp, i)); // was pressed, now is released

			LastMouseState.rgbButtons[i] = CurrentMouseState.rgbButtons[i];
		}
		else if (CurrentMouseState.rgbButtons[i] & 0x80) Binding = Bindings.find(CallbackInfo(CallbackInfo::OnMousePress, i));

		if (Binding != BindingsEnd) {
			EventDispatcher->addTask(std::bind(Binding->second, Binding->first.Parameter));
			Binding = BindingsEnd;
		}
	}
	LastMouseState = CurrentMouseState;

	DWORD XInputResult = XInputGetState(0, &CurrentXInputState);
	if (XInputResult == ERROR_SUCCESS) {
		Binding = BindingsEnd;
		for (DWORD i = XINPUT_GAMEPAD_DPAD_UP; i <= XINPUT_GAMEPAD_Y; i <<= 1) {
			if (LastXInputState.Gamepad.wButtons & i) {
				if (CurrentXInputState.Gamepad.wButtons & i) Binding = Bindings.find(CallbackInfo(CallbackInfo::OnGamepadPress, i));
				else Binding = Bindings.find(CallbackInfo(CallbackInfo::OnGamepadUp, i));
			}
			else if (CurrentXInputState.Gamepad.wButtons & i) Binding = Bindings.find(CallbackInfo(CallbackInfo::OnGamepadDown, i));

			if (Binding != BindingsEnd) {
				EventDispatcher->addTask(std::bind(Binding->second, Binding->first.Parameter));
				Binding = BindingsEnd;
			}
		}

		Binding = Bindings.find(CallbackInfo(CallbackInfo::OnGamepadLeftThumb));
		if (Binding != BindingsEnd) {
			EventDispatcher->addTask(std::bind(Binding->second, normalizeStickValues(CurrentXInputState.Gamepad.sThumbLX, CurrentXInputState.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)));
			Binding = BindingsEnd;
		}

		Binding = Bindings.find(CallbackInfo(CallbackInfo::OnGamepadRightThumb));
		if (Binding != BindingsEnd) {
			EventDispatcher->addTask(std::bind(Binding->second, normalizeStickValues(CurrentXInputState.Gamepad.sThumbRX, CurrentXInputState.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)));
			Binding = BindingsEnd;
		}

		std::memcpy(&LastXInputState, &CurrentXInputState, sizeof XINPUT_STATE);
	}
	else std::memset(&LastXInputState, 0, sizeof XINPUT_STATE);
}

bool Input::Manager::isKeyPressed(int key)
{
	return PressedKeys.find(key) != PressedKeys.end();
}

void Input::Manager::addBinding(CallbackInfo& Information, Callback Binding)
{
	Bindings[Information] = Binding;
}

void Input::Manager::removeBinding(CallbackInfo& Information)
{
	auto i = Bindings.find(Information);
	if (i != Bindings.end())
		Bindings.erase(i);
}

void Input::Manager::setMouseBinding(MouseMoveCallback Binding)
{
	MouseCallback = Binding;
}

void Input::Manager::removeMouseBinding()
{
	MouseCallback = nullptr;
}

Input::ThumbMovement* Input::Manager::normalizeStickValues(float ValueX, float ValueY, int Deadzone)
{
	//determine how far the controller is pushed
	float Magnitude = sqrt(ValueX*ValueX + ValueY*ValueY);

	//determine the direction the controller is pushed
	float NormalizedLX = ValueX / Magnitude;
	float NormalizedLY = ValueY / Magnitude;

	float NormalizedMagnitude = 0;

	//check if the controller is outside a circular dead zone
	if (Magnitude > Deadzone)
	{
		//clip the magnitude at its expected maximum value
		if (Magnitude > 32767) Magnitude = 32767;

		//adjust magnitude relative to the end of the dead zone
		Magnitude -= Deadzone;

		//optionally normalize the magnitude with respect to its expected range
		//giving a magnitude value of 0.0 to 1.0
		NormalizedMagnitude = Magnitude / (32767 - Deadzone);
	}
	else //if the controller is in the deadzone zero out the magnitude
	{
		Magnitude = 0.0;
		NormalizedMagnitude = 0.0;
	}

	return new ThumbMovement{ NormalizedLX * NormalizedMagnitude, NormalizedLY * NormalizedMagnitude };
}
