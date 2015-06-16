#pragma once

#include "PMXDefinitions.h"

#include <LogicComponent.h>

class PMXRigidBody : public Urho3D::LogicComponent
{
	OBJECT(PMXRigidBody);

public:
	PMXRigidBody(Urho3D::Context *context);
	~PMXRigidBody();

	virtual void FixedUpdate(float timeStep);
};

