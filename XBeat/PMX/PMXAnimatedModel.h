#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimatedModel.h>

class PMXModel;

struct PMXAnimatedModel {
    static void SetModel(PMXModel *model, Urho3D::AnimatedModel *animModel, const Urho3D::String &baseDir);
};
