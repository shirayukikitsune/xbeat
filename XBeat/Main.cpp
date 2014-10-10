//===-- Main.cpp - Defines the windows application entrypoint --------*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines the windows application entrypoint
///
//===---------------------------------------------------------------------------===//

#include "SystemClass.h"

#include <cassert>

int WINAPI WinMain(HINSTANCE CurrentInstance, HINSTANCE PreviousInstance, PSTR CommandLine, int DisplayCommand)
{
	if (auto System = SystemClass::getInstance().lock()) {
		assert(System->initialize() == true);

		System->run();

		System->shutdown();

		return 0;
	}

	return 1;
}
