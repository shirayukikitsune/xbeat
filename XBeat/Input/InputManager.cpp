#include "InputManager.h"
#include <queue>

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
	
	this->dispatcher = dispatcher;

	memset(keyState, 0, sizeof(keyState));

	result = DirectInput8Create(instance, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&dinput, NULL);
	if (FAILED(result))
		return false;

	result = dinput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	if (FAILED(result))
		return false;

	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(result))
		return false;

	result = keyboard->SetCooperativeLevel(wnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (FAILED(result))
		return false; 

	DIPROPDWORD prop;
	prop.diph.dwSize = sizeof(DIPROPDWORD);
	prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	prop.diph.dwObj = 0;
	prop.diph.dwHow = DIPH_DEVICE;
	prop.dwData = keyboardBufferSize; 

	result = keyboard->SetProperty(DIPROP_BUFFERSIZE, &prop.diph);
	if (FAILED(result))
		return false;

	keyboard->Acquire();

	result = dinput->CreateDevice(GUID_SysMouse, &mouse, NULL);
	if (FAILED(result))
		return false;

	result = mouse->SetDataFormat(&c_dfDIMouse2);
	if (FAILED(result))
		return false;

	result = mouse->SetCooperativeLevel(wnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (FAILED(result))
		return false;

	mouse->Acquire();

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

	result = keyboard->GetDeviceState(sizeof (keyState), (LPVOID)&keyState);
	if (FAILED(result)) {
		if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED)
			return !(FAILED(keyboard->Acquire()));
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
			return !(FAILED(mouse->Acquire()));
		else
			return false;
	}

	return true;
}

void Manager::ProcessInput()
{
	DIDEVICEOBJECTDATA keyChanges[keyboardBufferSize];
	DWORD items = keyboardBufferSize;
	HRESULT result;
	result = keyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), keyChanges, &items, 0);
	auto v = bindings.end();
	std::queue<uint8_t> upEvents;

	if (!FAILED(result)) {
		for (DWORD i = 0; i < items; i++) {
			pressedKeys[(uint8_t)keyChanges[i].dwOfs] = (keyChanges[i].dwData & 0x80) != 0 ? CallbackInfo::OnKeyDown : CallbackInfo::OnKeyUp;
		}
	}

	for (auto &key : pressedKeys) {
		auto v = bindings.find(CallbackInfo(key.second, key.first));
		if (v != bindings.end())
			dispatcher->AddTask(std::bind(v->second, v->first.param));
		if (key.second == CallbackInfo::OnKeyDown)
			key.second = CallbackInfo::OnKeyPressed;
		else if (key.second == CallbackInfo::OnKeyUp) upEvents.push(key.first);
	}

	while (!upEvents.empty()) {
		pressedKeys.erase(upEvents.front());
		upEvents.pop();
	}

	// Check for mouse movements
	if (mouseCallback) {
		std::shared_ptr<MouseMovement> m(new MouseMovement);
		m->x = currentMouseState.lX;
		m->y = currentMouseState.lY;
		this->dispatcher->AddTask(std::bind(mouseCallback, m));
	}
	for (int i = 0; i < 8; i++)
	{
		if (lastMouseState.rgbButtons[i] != currentMouseState.rgbButtons[i]) {
			if (currentMouseState.rgbButtons[i] & 0x80) // Was released, now is pressed
				v = bindings.find(CallbackInfo(CallbackInfo::OnMouseDown, i));
			else v = bindings.find(CallbackInfo(CallbackInfo::OnMouseUp, i)); // was pressed, now is released

			lastMouseState.rgbButtons[i] = currentMouseState.rgbButtons[i];
		}
		else if (currentMouseState.rgbButtons[i] & 0x80) v = bindings.find(CallbackInfo(CallbackInfo::OnMousePress, i));

		if (v != bindings.end()) {
			this->dispatcher->AddTask(std::bind(v->second, v->first.param));
			v = bindings.end();
		}
	}
	lastMouseState = currentMouseState;
}

bool Manager::IsEscapePressed()
{
	return IsKeyPressed(DIK_ESCAPE);
}

bool Manager::IsKeyPressed(int key)
{
	return pressedKeys.find(key) != pressedKeys.end();
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
