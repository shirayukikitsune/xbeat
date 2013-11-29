#pragma once

#include <bitset>
#include <cstdint>
#include <map>
#include <memory>

#define DIRECTINPUT_VERSION 0x0800

#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

#include <dinput.h>
#include "../Dispatcher.h"

namespace Input {

struct MouseMovement
{
	long x;
	long y;
};

struct CallbackInfo
{
	enum Event {
		OnKeyUp,
		OnKeyDown,
		OnMouseMove,
		OnMouseClick,
		OnMouseDoubleClick
	};
	Event eventType;
	uint8_t button;
	void *param;

	CallbackInfo(Event event, uint8_t btn = 0, void *prm = nullptr) : eventType(event), button(btn), param(prm) {};

};
bool operator< (const CallbackInfo& one, const CallbackInfo& other);
bool operator== (const CallbackInfo& one, const CallbackInfo& other);

typedef std::function<void(void*)> Callback;
typedef std::function<void(std::shared_ptr<MouseMovement>)> MouseMoveCallback;

class Manager
{
public:
	Manager();
	~Manager();

	bool Initialize(HINSTANCE instance, HWND wnd, int width, int height, std::shared_ptr<Dispatcher> dispatcher);
	void Shutdown();
	bool Frame();

	bool IsEscapePressed();
	bool IsKeyPressed(int key);

	void GetMouseLocation(int &x, int &y);

	void AddBinding(CallbackInfo &info, Callback callback);
	void RemoveBinding(CallbackInfo &info);

	void SetMouseBinding(MouseMoveCallback callback);
	void RemoveMouseBinding();

private:
	bool ReadKeyboard();
	bool ReadMouse();
	void ProcessInput();

	IDirectInput8 *dinput;
	IDirectInputDevice8 *keyboard;
	IDirectInputDevice8 *mouse;

	int screenWidth, screenHeight;
	int mouseX, mouseY;

	uint8_t currentKeyState[256], lastKeysState[256];
	DIMOUSESTATE2 currentMouseState, lastMouseState;

	MouseMoveCallback mouseCallback;
	std::map<CallbackInfo, Callback> bindings;

	std::shared_ptr<Dispatcher> dispatcher;
};

}
