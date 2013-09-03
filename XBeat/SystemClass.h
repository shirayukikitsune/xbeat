#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <memory>

#include "Input/Manager.h"
#include "Renderer/Manager.h"

class SystemClass
{
public:
	static std::weak_ptr<SystemClass> getInstance() {
		static std::shared_ptr<SystemClass> instance;
		if (instance.get() == nullptr) {
			instance.reset(new SystemClass);
		}
		return instance;
	}

	~SystemClass();

	bool Initialize();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);

private:
	void InitializeWindow(int &width, int &height);
	bool Frame();
	void ShutdownWindow();

	SystemClass();
	SystemClass(const SystemClass &);

private: 
	LPCWSTR appName;
	HINSTANCE instance;
	HWND wnd;

	std::shared_ptr<Input::Manager> input;
	std::shared_ptr<Renderer::Manager> renderer;
};


LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam);
