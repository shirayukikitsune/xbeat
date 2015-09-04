#include "PMXRigidBody.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Node.h>

PMXRigidBody::PMXRigidBody(Urho3D::Context *context)
	: Urho3D::LogicComponent(context)
{
	SetUpdateEventMask(Urho3D::USE_FIXEDPOSTUPDATE);
}


PMXRigidBody::~PMXRigidBody()
{
}

void PMXRigidBody::RegisterObject(Urho3D::Context * context)
{
	context->RegisterFactory<PMXRigidBody>();
}

void PMXRigidBody::FixedPostUpdate(float timeStep)
{
	auto rb = GetComponent<Urho3D::RigidBody>();
	//rb->SetPosition(node_->GetWorldPosition());
}
