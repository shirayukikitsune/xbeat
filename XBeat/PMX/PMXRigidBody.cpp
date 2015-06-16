#include "PMXRigidBody.h"

#include <Node.h>
#include <RigidBody.h>

PMXRigidBody::PMXRigidBody(Urho3D::Context *context)
	: Urho3D::LogicComponent(context)
{
	SetUpdateEventMask(Urho3D::USE_FIXEDPOSTUPDATE);
}


PMXRigidBody::~PMXRigidBody()
{
}

void PMXRigidBody::FixedUpdate(float timeStep)
{
	auto rb = GetComponent<Urho3D::RigidBody>();
	rb->SetPosition(node_->GetWorldPosition());
}
