#pragma once

#include "PMXDefinitions.h"

#include <Urho3D/Urho3D.h>
#include <Urho3D/Scene/LogicComponent.h>

class PMXRigidBody : public Urho3D::LogicComponent
{
	URHO3D_OBJECT(PMXRigidBody, Urho3D::LogicComponent);

public:
	PMXRigidBody(Urho3D::Context *context);
	~PMXRigidBody();
	/// Register object factory. Drawable must be registered first.
	static void RegisterObject(Urho3D::Context* context);

	virtual void FixedPostUpdate(float timeStep);
};

