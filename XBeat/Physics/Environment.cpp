//===-- Physics/Environment.cpp - Defines the physics environment ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines everything related to the physics environment class,
/// which manages all rigid bodies, soft bodies and constraints.
///
//===---------------------------------------------------------------------------===//

#include "Environment.h"

#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodySolvers.h>

#include <algorithm>
#include <cassert>

Physics::Environment::Environment()
{
	State = SimulationState::Running;
	PauseTime = 0.0f;
}

Physics::Environment::~Environment()
{
	shutdown();
}

// Create all pointers to the bullet world and set the default parameters
void Physics::Environment::initialize()
{
	Broadphase.reset(new btDbvtBroadphase);
	assert(Broadphase != nullptr);

	CollisionConfiguration.reset(new btSoftBodyRigidBodyCollisionConfiguration);
	assert(CollisionConfiguration != nullptr);

	CollisionDispatcher.reset(new btCollisionDispatcher(CollisionConfiguration.get()));
	assert(CollisionDispatcher != nullptr);

	ConstraintSolver.reset(new btSequentialImpulseConstraintSolver);
	assert(ConstraintSolver != nullptr);

	SoftBodySolver.reset(new btDefaultSoftBodySolver);
	assert(SoftBodySolver != nullptr);

	DynamicsWorld.reset(new btSoftRigidDynamicsWorld(CollisionDispatcher.get(), Broadphase.get(), ConstraintSolver.get(), CollisionConfiguration.get(), SoftBodySolver.get()));
	assert(DynamicsWorld != nullptr);

	// Here the gravity force is multiplied by 10 due the approximation that 10u in PMD/PMX models = 1m
	DynamicsWorld->setGravity(btVector3(0, -9.8f, 0));
}

// Before deleting the pointers to the bullet world, we must remove all registered bodies and contraints.
void Physics::Environment::shutdown()
{
	for (auto i = SoftBodies.begin(); !SoftBodies.empty(); i = SoftBodies.begin()) {
		DynamicsWorld->removeSoftBody(i->get());
		SoftBodies.erase(i);
	}
	for (auto i = Constraints.begin(); !Constraints.empty(); i = Constraints.begin()) {
		DynamicsWorld->removeConstraint(i->get());
		Constraints.erase(i);
	}
	for (auto i = RigidBodies.begin(); !RigidBodies.empty(); i = RigidBodies.begin()) {
		DynamicsWorld->removeRigidBody(i->get());
		RigidBodies.erase(i);
	}

	// Delete the created pointers in reverse order
	DynamicsWorld.reset();
	SoftBodySolver.reset();
	ConstraintSolver.reset();
	CollisionDispatcher.reset();
	CollisionConfiguration.reset();
	Broadphase.reset();
}

void Physics::Environment::doFrame(float Time)
{
	if (!isRunning()) {
		// If we are on hold, increase the time counter, so when we resume, we can skip the simulation for this long
		if (isHolding())
			PauseTime += Time;

		return;
	}

	// Check if the simulation was on hold
	if (PauseTime > 0.0f) {
		// If we were on hold, do all simulation while we were frozen on a single step
		DynamicsWorld->stepSimulation(PauseTime / 1000.f, 1);

		// We must not skip the current frame simulation, so dont exit yet!
		PauseTime = 0.0f;
	}

	// Do our frame simulation
	DynamicsWorld->stepSimulation(Time);
}

void Physics::Environment::addSoftBody(std::shared_ptr<btSoftBody> SoftBody, int16_t Group, int16_t Mask)
{
	SoftBodies.insert(SoftBody);
	DynamicsWorld->addSoftBody(SoftBody.get(), Group, Mask);
}

void Physics::Environment::removeSoftBody(std::shared_ptr<btSoftBody> SoftBody)
{
	auto i = SoftBodies.find(SoftBody);

	if (i != SoftBodies.end()) {
		SoftBodies.erase(i);
		DynamicsWorld->removeSoftBody(SoftBody.get());
	}
}

void Physics::Environment::addRigidBody(std::shared_ptr<btRigidBody> RigidBody, int16_t Group, int16_t Mask)
{
	RigidBodies.insert(RigidBody);
	DynamicsWorld->addRigidBody(RigidBody.get(), Group, Mask);
}

void Physics::Environment::removeRigidBody(std::shared_ptr<btRigidBody> RigidBody)
{
	auto i = RigidBodies.find(RigidBody);

	if (i != RigidBodies.end()) {
		RigidBodies.erase(i);
		DynamicsWorld->removeRigidBody(RigidBody.get());
	}
}

void Physics::Environment::addConstraint(std::shared_ptr<btTypedConstraint> Constraint)
{
	Constraints.insert(Constraint);
	DynamicsWorld->addConstraint(Constraint.get(), true);
}

void Physics::Environment::removeConstraint(std::shared_ptr<btTypedConstraint> Constraint)
{
	auto i = Constraints.find(Constraint);

	if (i != Constraints.end()) {
		Constraints.erase(i);
		DynamicsWorld->removeConstraint(Constraint.get());
	}
}
