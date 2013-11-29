#include "InputManager.h"

using namespace Input;


bool Input::operator< (const CallbackInfo& one, const CallbackInfo& other) {
	if (one.eventType < other.eventType) return true;
	if (one.eventType == other.eventType && one.button < other.button) return true;

	return false;
}
bool Input::operator== (const CallbackInfo& one, const CallbackInfo& other) {
	return one.eventType == other.eventType && one.button == other.button;
}

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


bool Manager::Initialize(HINSTANCE instance, HWND wnd, int width, int height, std::shared_ptr<Dispatcher> dispatcher)
{
	HRESULT result;

	this->screenWidth = width;
	this->screenHeight = height;
	
	this->mouseX = this->mouseY = 0;

	this->dispatcher = dispatcher;

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

	result = mouse->SetDataFormat(&c_dfDIMouse2);
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
	return true;
}

bool Manager::ReadKeyboard()
{
	HRESULT result;

	result = keyboard->GetDeviceState(sizeof (currentKeyState), (LPVOID)&currentKeyState);
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

	result = mouse->GetDeviceState(sizeof (DIMOUSESTATE2), (LPVOID)&currentMouseState);
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
	mouseX += currentMouseState.lX;
	mouseY += currentMouseState.lY;

	// Ensure the mouse location doesn't exceed the screen width or height.
	if(mouseX < 0)  { mouseX = 0; }
	if(mouseY < 0)  { mouseY = 0; }
	
	if(mouseX > screenWidth)  { mouseX = screenWidth; }
	if(mouseY > screenHeight) { mouseY = screenHeight; }

	auto v = bindings.end();
	// Check for key strokes
	for (int i = 0; i < 256; i++) {
		if (lastKeysState[i] != currentKeyState[i]) {
			// Key state changed
			if (IsKeyPressed(i)) // Was released, now is pressed
				v = bindings.find(CallbackInfo(CallbackInfo::OnKeyDown, i));
			else v = bindings.find(CallbackInfo(CallbackInfo::OnKeyUp, i)); // was pressed, now is released

			if (v != bindings.end())
				this->dispatcher->AddTask(std::bind(v->second, v->first.param));

			lastKeysState[i] = currentKeyState[i];
		}
	}

	// Check for mouse movements
	if (mouseCallback) {
		std::shared_ptr<MouseMovement> m(new MouseMovement);
		m->x = currentMouseState.lX;
		m->y = currentMouseState.lY;
		this->dispatcher->AddTask(std::bind(mouseCallback, m));
	}
}

bool Manager::IsEscapePressed()
{
	return IsKeyPressed(DIK_ESCAPE);
}

bool Manager::IsKeyPressed(int key)
{
	return (currentKeyState[key] & 0x80) != 0;
}

void Manager::GetMouseLocation(int &x, int &y)
{
	x = mouseX;
	y = mouseY;
}

void Manager::AddBinding(CallbackInfo& info, Callback callback)
{
	bindings[info] = callback;
}

void Manager::RemoveBinding(CallbackInfo& info)
{
	auto i = bindings.find(info);
	if (i != bindings.end())
		bindings.erase(i);
}

void Manager::SetMouseBinding(MouseMoveCallback callback)
{
	this->mouseCallback = callback;
}

void Manager::RemoveMouseBinding()
{
	this->mouseCallback = nullptr;
}
