#include "Manager.h"

using namespace Input;

Manager::Manager(void)
{
	dinput = nullptr;
	keyboard = nullptr;
	mouse = nullptr;
}


Manager::~Manager(void)
{
	Shutdown();
}


bool Manager::Initialize(HINSTANCE instance, HWND wnd, int width, int height)
{
	HRESULT result;

	this->screenWidth = width;
	this->screenHeight = height;
	
	this->mouseX = this->mouseY = 0;

	result = DirectInput8Create(instance, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&dinput, NULL);
	if (FAILED(result))
		return false;

	result = dinput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	if (FAILED(result))
		return false;

	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(result))
		return false;

	result = keyboard->SetCooperativeLevel(wnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
		return false;

	result = keyboard->Acquire();
	if (FAILED(result))
		return false;

	result = dinput->CreateDevice(GUID_SysMouse, &mouse, NULL);
	if (FAILED(result))
		return false;

	result = mouse->SetDataFormat(&c_dfDIMouse);
	if (FAILED(result))
		return false;

	result = mouse->SetCooperativeLevel(wnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
		return false;

	result = mouse->Acquire();
	if (FAILED(result))
		return false;

	return true;
}

void Manager::Shutdown()
{
	if (mouse) {
		mouse->Unacquire();
		mouse->Release();
		mouse = nullptr;
	}

	if (keyboard) {
		keyboard->Unacquire();
		keyboard->Release();
		keyboard = nullptr;
	}

	if (dinput) {
		dinput->Release();
		dinput = nullptr;
	}
}

bool Manager::Frame()
{
	if (!ReadKeyboard())
		return false;

	if (!ReadMouse())
		return false;

	ProcessInput();
}

bool Manager::ReadKeyboard()
{
	HRESULT result;

	result = keyboard->GetDeviceState(sizeof (keys), (LPVOID)&keys);
	if (FAILED(result)) {
		if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED)
			keyboard->Acquire();
		else
			return false;
	}

	return true;
}

bool Manager::ReadMouse()
{
	HRESULT result;

	result = mouse->GetDeviceState(sizeof (DIMOUSESTATE), (LPVOID)&mouseState);
	if (FAILED(result)) {
		if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED)
			mouse->Acquire();
		else
			return false;
	}

	return true;
}

void Manager::ProcessInput()
{
	// Update the location of the mouse cursor based on the change of the mouse location during the frame.
	mouseX += mouseState.lX;
	mouseY += mouseState.lY;

	// Ensure the mouse location doesn't exceed the screen width or height.
	if(mouseX < 0)  { mouseX = 0; }
	if(mouseY < 0)  { mouseY = 0; }
	
	if(mouseX > screenWidth)  { mouseX = screenWidth; }
	if(mouseY > screenHeight) { mouseY = screenHeight; }
}

bool Manager::IsEscapePressed()
{
	return IsKeyPressed(DIK_ESCAPE);
}

bool Manager::IsKeyPressed(int key)
{
	return (keys[key] & 0x80) != 0;
}

void Manager::GetMouseLocation(int &x, int &y)
{
	x = mouseX;
	y = mouseY;
}
