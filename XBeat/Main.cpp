#include "SystemClass.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine, int cmdShow)
{
	if (auto system = SystemClass::getInstance().lock()) {
		bool result = system->Initialize();
		if (result) {
			system->Run();
		}

		system->Shutdown();

		return 0;
	}

	return 1;
}
