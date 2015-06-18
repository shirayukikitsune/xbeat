#include "PMXIKNode.h"

#include <Context.h>
#include <Variant.h>

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

	ATTRIBUTE(PMXIKNode, VAR_BOOL, "Limit Angles", limited, false, AM_DEFAULT);
	ATTRIBUTE(PMXIKNode, VAR_VECTOR3, "Lower Limit", lowerLimit, Vector3::ZERO, AM_DEFAULT);
	ATTRIBUTE(PMXIKNode, VAR_VECTOR3, "Upper Limit", upperLimit, Vector3::ZERO, AM_DEFAULT);
}

