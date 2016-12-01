#include "PMXIKNode.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Variant.h>

PMXIKNode::PMXIKNode(Urho3D::Context *context)
	: Urho3D::Component(context)
{
	limited = false;
	boneLength = 0.0f;
}


PMXIKNode::~PMXIKNode()
{
}

void PMXIKNode::RegisterObject(Urho3D::Context *context)
{
	using namespace Urho3D;
	context->RegisterFactory<PMXIKNode>();

    URHO3D_ATTRIBUTE("Limit Angles", bool, limited, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Lower Limit", Vector3, lowerLimit, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Upper Limit", Vector3, upperLimit, Vector3::ZERO, AM_DEFAULT);
}

