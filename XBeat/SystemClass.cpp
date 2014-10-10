//===-- SystemClass.cpp - Defines the functional entrypoint of the engine --*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the functional entrypoint of the
/// engine class
///
//===---------------------------------------------------------------------------------===//

#include "SystemClass.h"

#include "Dispatcher.h"
#include "Input/InputManager.h"
// Renderer must be placed before Physics due to incompatibilities of Bullet and DirectX
#include "Renderer/SceneManager.h"
#include "Physics/Environment.h"

#include <cassert>
#include <chrono>

SystemClass::SystemClass()
{
	Window = NULL;
	ApplicationInstance = NULL;
}


bool SystemClass::initialize()
{
	int Width, Height;

	initializeWindow(Width, Height);

	EventDispatcher.reset(new Dispatcher);
	assert(EventDispatcher);
	EventDispatcher->initialize();

	InputManager.reset(new Input::Manager);
	assert(InputManager);
	if (!InputManager->initialize(ApplicationInstance, Window, EventDispatcher))
	{
		MessageBox(Window, L"Failed to initialize DirectInput8 interface", L"Error", MB_OK);
		return false;
	}

	// Sets the initial timer to zero
	calculateFrameMsec();

	RendererManager.reset(new Renderer::SceneManager);
	assert(RendererManager);

	PhysicsWorld.reset(new Physics::Environment);
	assert(PhysicsWorld);

	if (!RendererManager->Initialize(Width, Height, Window, InputManager, PhysicsWorld, EventDispatcher))
		return false;

	PhysicsWorld->initialize();

	if (!RendererManager->LoadScene())
		return false;

	return true;
}

void SystemClass::run()
{
	MSG Message;
	bool Done = false;

	ZeroMemory(&Message, sizeof MSG);

	while (!Done) {
		if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}

		if (Message.message == WM_QUIT || !doFrame())
			Done = true;

		if (InputManager->isKeyPressed(DIK_ESCAPE))
			Done = true;
	}
}

void SystemClass::shutdown()
{
	RendererManager.reset();
	InputManager.reset();
	EventDispatcher.reset();

	shutdownWindow();
}

bool SystemClass::doFrame()
{
	float frameTime = calculateFrameMsec();

	if (!InputManager->doFrame())
		return false;

	PhysicsWorld->doFrame(frameTime);

	if (!RendererManager->Frame(frameTime))
		return false;

	return true;
}

void SystemClass::initializeWindow(int &width, int &height)
{
	assert(Window == NULL);

	WNDCLASSEX WindowClass;

	ApplicationInstance = GetModuleHandle(NULL);
	ApplicationName = L"XBeat";

	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc = WndProc;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = ApplicationInstance;
	WindowClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	WindowClass.hIconSm = WindowClass.hIcon;
	WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WindowClass.lpszMenuName = NULL;
	WindowClass.lpszClassName = ApplicationName;
	WindowClass.cbSize = sizeof WNDCLASSEX;

	RegisterClassEx(&WindowClass);

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	int posx = 0, posy = 0;

	if (Renderer::FULL_SCREEN) {
		DEVMODE DisplaySettings;
		ZeroMemory(&DisplaySettings, sizeof DisplaySettings);
		DisplaySettings.dmSize = sizeof DisplaySettings;
		DisplaySettings.dmPelsWidth = (DWORD)width;
		DisplaySettings.dmPelsHeight = (DWORD)height;
		DisplaySettings.dmBitsPerPel = 32;
		DisplaySettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		ChangeDisplaySettings(&DisplaySettings, CDS_FULLSCREEN);
	}
	else {
		posx = (width - 1280) / 2;
		posy = (height - 720) / 2;

		width = 1280;
		height = 720;
	}

	Window = CreateWindowEx(WS_EX_APPWINDOW, ApplicationName, ApplicationName, WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP,
		posx, posy, width, height, NULL, NULL, ApplicationInstance, NULL);

	ShowWindow(Window, SW_SHOW);
	SetForegroundWindow(Window);
	SetFocus(Window);
}

void SystemClass::shutdownWindow()
{
	assert(Window != NULL);

	if (Renderer::FULL_SCREEN)
		ChangeDisplaySettings(NULL, 0);

	DestroyWindow(Window);
	Window = NULL;

	UnregisterClass(ApplicationName, ApplicationInstance);
	ApplicationInstance = NULL;
}

float SystemClass::calculateFrameMsec()
{
	static std::chrono::high_resolution_clock::time_point LastTime = std::chrono::high_resolution_clock::now();

	auto NowTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> ElapsedSec = NowTime - LastTime;
	LastTime = NowTime;

	return ElapsedSec.count() * 1000.0f;
}

LRESULT CALLBACK SystemClass::messageHandler(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	return DefWindowProc(Window, Message, WParam, LParam);
}

LRESULT CALLBACK WndProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	switch (Message)
	{
		// Check if the window is being destroyed.
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}

		// Check if the window is being closed.
		case WM_CLOSE:
		{
			PostQuitMessage(0);		
			return 0;
		}

		// All other messages pass to the message handler in the system class.
		default:
		{
			if (auto system = SystemClass::getInstance().lock())
				return system->messageHandler(Window, Message, WParam, LParam);
			else return DefWindowProc(Window, Message, WParam, LParam);
		}
	}
}
