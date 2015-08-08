#include "PMXRigidBody.h"

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

void PMXRigidBody::FixedPostUpdate(float timeStep)
{
	auto rb = GetComponent<Urho3D::RigidBody>();
	//rb->SetPosition(node_->GetWorldPosition());
}
