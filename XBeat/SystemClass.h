//===-- SystemClass.h - Declares the functional entrypoint of the engine --*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the functional entrypoint of the
/// engine class
///
//===--------------------------------------------------------------------------------===//

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <memory>
#include <Windows.h>

class Dispatcher;
namespace Input { class Manager; }
namespace Physics { class Environment; }
namespace Scenes { class SceneManager; }

/// \brief An interface to initialize the engine
class SystemClass
{
public:
	static std::weak_ptr<SystemClass> getInstance() {
		static std::shared_ptr<SystemClass> Instance;

		if (Instance.get() == nullptr) {
			Instance.reset(new SystemClass);
		}

		return Instance;
	}

	/// \brief Initializes the engine
	bool initialize();
	/// \brief Stops the engine
	void shutdown();
	/// \brief Run the engine
	void run();

	/// \brief Handler for Windows messages
	LRESULT CALLBACK messageHandler(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

private:
	/// \brief Initializes and creates a window
	void initializeWindow(int &Width, int &Height);
	/// \brief Step of execution of the engine
	bool doFrame();
	/// \brief Destroys the window created by SystemClass::initializeWindow()
	void shutdownWindow();

	/// \brief Calculates the time span, in milliseconds, between the start
	/// of the last frame and the start of the current frame
	float calculateFrameTime();

	SystemClass();
	SystemClass(const SystemClass &) = delete;

private:
	/// \brief The name of the application
	LPCWSTR ApplicationName;
	/// \brief The instance of the application
	HINSTANCE ApplicationInstance;
	/// \brief The window of the application
	HWND Window;

	/// \brief The instance of the Input::Manager
	std::shared_ptr<Input::Manager> InputManager;
	/// \brief The instance of the Renderer::SceneManager
	std::shared_ptr<Scenes::SceneManager> SceneManager;
	/// \brief The instance of the Physics::Environment
	std::shared_ptr<Physics::Environment> PhysicsWorld;
	/// \brief The instance of the Dispatcher
	std::shared_ptr<Dispatcher> EventDispatcher;
};

/// \brief Default window message handler
LRESULT CALLBACK WndProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);
