#include "SystemClass.h"
#include <tchar.h>
#include <chrono>

SystemClass::SystemClass(void)
{
	input.reset();
	renderer.reset();
}


SystemClass::SystemClass(const SystemClass &other)
{
}


SystemClass::~SystemClass(void)
{
}


bool SystemClass::Initialize()
{
	int width, height;

	InitializeWindow(width, height);

	dispatcher.reset(new Dispatcher);
	if (dispatcher == nullptr)
		return false;
	dispatcher->Initialize();

	input.reset(new Input::Manager);
	if (input == nullptr)
		return false;

	if (!input->Initialize(instance, wnd, width, height, dispatcher))
	{
		MessageBox(wnd, L"Failed to initialize DirectInput8 interface", L"Error", MB_OK);
		return false;
	}

	frameMsec();

	renderer.reset(new Renderer::SceneManager);
	if (renderer == nullptr)
		return false;

	physics.reset(new Physics::Environment);
	if (physics == nullptr)
		return false;

	if (!renderer->Initialize(width, height, wnd, input, physics, dispatcher))
		return false;

	if (!physics->Initialize(/*renderer->GetRenderer()*/nullptr))
		return false;

	if (!renderer->LoadScene())
		return false;

	return true;
}

void SystemClass::Run()
{
	MSG msg;
	bool done = false;

	ZeroMemory(&msg, sizeof MSG);

	while (!done) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT || !Frame())
			done = true;

		if (input->IsEscapePressed())
			done = true;
	}
}

void SystemClass::Shutdown()
{
	if (renderer != nullptr) {
		renderer->Shutdown();
		renderer.reset();
	}

	if (input != nullptr) {
		input->Shutdown();
		input.reset();
	}

	if (dispatcher != nullptr) {
		dispatcher->Shutdown();
		dispatcher.reset();
	}

	ShutdownWindow();
}

bool SystemClass::Frame()
{
	float frameTime = frameMsec();

	if (!input->Frame())
		return false;

	if (!physics->Frame(frameTime))
		return false;

	if (!renderer->Frame(frameTime))
		return false;

	return true;
}

void SystemClass::InitializeWindow(int &width, int &height)
{
	WNDCLASSEX wc;

	instance = GetModuleHandle(NULL);
	appName = L"XBeat";

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = instance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = appName;
	wc.cbSize = sizeof WNDCLASSEX;

	RegisterClassEx(&wc);

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	int posx = 0, posy = 0;

	if (Renderer::FULL_SCREEN) {
		DEVMODE dm;
		ZeroMemory(&dm, sizeof dm);
		dm.dmSize = sizeof dm;
		dm.dmPelsWidth = (DWORD)width;
		dm.dmPelsHeight = (DWORD)height;
		dm.dmBitsPerPel = 32;
		dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
	}
	else {
		posx = (width - 800) / 2;
		posy = (height - 600) / 2;

		width = 800;
		height = 600;
	}

	wnd = CreateWindowEx(WS_EX_APPWINDOW, appName, appName, WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP,
		posx, posy, width, height, NULL, NULL, instance, NULL);

	ShowWindow(wnd, SW_SHOW);
	SetForegroundWindow(wnd);
	SetFocus(wnd);

	ShowCursor(false);
}

void SystemClass::ShutdownWindow()
{
	ShowCursor(true);

	if (Renderer::FULL_SCREEN)
		ChangeDisplaySettings(NULL, 0);

	DestroyWindow(wnd);
	wnd = NULL;

	UnregisterClass(appName, instance);
	instance = NULL;
}

float SystemClass::frameMsec()
{
	static std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

	auto nowTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> elapsedSec = nowTime - lastTime;
	lastTime = nowTime;

	return elapsedSec.count() * 1000.0f;
}

LRESULT CALLBACK SystemClass::MessageHandler(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(wnd, msg, wparam, lparam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch(umessage)
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
				return system->MessageHandler(hwnd, umessage, wparam, lparam);
			else return DefWindowProc(hwnd, umessage, wparam, lparam);
		}
	}
}
