//===-- PMX/PMXLoader.h - Declares the PMX loading class ------------*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares the PMX::Loader class
///
//===--------------------------------------------------------------------------===//

#pragma once

#include "PMXDefinitions.h"

#include <exception>
#include <fstream>
#include <istream>
#include <string>

#include <Ptr.h>

namespace Urho3D {
	class AnimatedModel;
}

namespace PMX {

class Loader {
public:
	bool loadFromFile(Urho3D::AnimatedModel* Model, const std::wstring &Filename);
	bool loadFromStream(Urho3D::AnimatedModel* Model, std::istream &InStream);
	bool loadFromMemory(Urho3D::AnimatedModel* Model, const char *&Data);
	ModelDescription getDescription(const std::wstring &Filename);

private:
};

}
