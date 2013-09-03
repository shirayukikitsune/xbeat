#pragma once

#include <bitset>
#include <cstdint>

#define DIRECTINPUT_VERSION 0x0800

#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

#include <dinput.h>

namespace Input {

class Manager
{
public:
	Manager();
	~Manager();

	bool Initialize(HINSTANCE instance, HWND wnd, int width, int height);
	void Shutdown();
	bool Frame();

	bool IsEscapePressed();
	bool IsKeyPressed(int key);

	void GetMouseLocation(int &x, int &y);

private:
	bool ReadKeyboard();
	bool ReadMouse();
	void ProcessInput();

	IDirectInput8 *dinput;
	IDirectInputDevice8 *keyboard;
	IDirectInputDevice8 *mouse;

	DIMOUSESTATE mouseState;

	int screenWidth, screenHeight;
	int mouseX, mouseY;

	uint8_t keys[256];
};

}
