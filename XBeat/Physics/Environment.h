//===-- Physics/Environment.h - Declares the physics environment ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the physics environment class,
/// which manages all rigid bodies, soft bodies and constraints.
///
//===--------------------------------------------------------------------------===//

#pragma once

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodySolvers.h>

#include <memory>
#include <set>

namespace Physics {

/// \brief Defines the physics simulation state
enum struct SimulationState {
	/// \brief The simulation is running normally
	Running,
	/// \brief The simulation is paused
	Paused,
	/// \brief The simulation is on hold
	///
	/// When on hold, the simulation timer will advance, but the bodies won't be updated. When resumed, the bodies will move to the positions as if the simulation was running normally.
	Holding
};

/// \brief The physics simulation environment
class Environment
{
public:
	Environment();
	~Environment();

	/// \brief Initializes the simulated world
	void initialize();
	/// \brief Stops the simulated world
	void shutdown();

	/// \brief Advances the simulation by a time step
	///
	/// \param [in] Time The time step to advance the simulation, in milliseconds
	void doFrame(float Time);

	/// \brief Pauses the simulated world
	void pause() { State = SimulationState::Paused; }
	/// \brief Puts the simulated world in hold
	///
	/// By putting the simulated world in hold, the time counter will continue to advance normally.
	///
	/// When the state is set to Running again, the simulation will be computed normally as if it were running normally.
	void hold() { State = SimulationState::Holding; }
	/// \brief Resumes the simulated world
	void resume() { State = SimulationState::Running; }
	/// \brief Checks if the simulated world is running
	bool isRunning() { return State == SimulationState::Running; }
	/// \brief Checks if the simulated world is paused
	bool isPaused() { return State == SimulationState::Paused; }
	/// \brief Checks if the simulated world is on hold
	/// \see Environemnt::hold()
	bool isHolding() { return State == SimulationState::Holding; }

	/// \brief Adds a soft body to the simulation
	///
	/// \param [in] SoftBody The soft body to be added
	/// \param [in] Group The collision group that the body will belong to
	/// \param [in] Mask The collision groups that the body will collide against
	void addSoftBody(std::shared_ptr<btSoftBody> SoftBody, int16_t Group, int16_t Mask);
	/// \brief Removes a soft body from the simulation
	void removeSoftBody(std::shared_ptr<btSoftBody> SoftBody);

	/// \brief Adds a rigid body to the simulation
	///
	/// \param [in] RigidBody The rigid body to be added
	/// \param [in] Group The collision group that the body will belong to
	/// \param [in] Mask The collision groups that the body will collide against
	void addRigidBody(std::shared_ptr<btRigidBody> RigidBody, int16_t Group = -1, int16_t Mask = -1);
	/// \brief Removes a rigid body from the simulation
	void removeRigidBody(std::shared_ptr<btRigidBody> RigidBody);

	/// \brief Adds a constraint to the simulation
	void addConstraint(std::shared_ptr<btTypedConstraint> Constraint);
	/// \brief Removes a constraint from the simulation
	void removeConstraint(std::shared_ptr<btTypedConstraint> Constraint);

private:
	/// \brief The current state of the simulation
	SimulationState State;
	/// \brief The time that the simulation is on hold
	float PauseTime;

	/// \brief The soft bodies that are registered in this world
	std::set<std::shared_ptr<btSoftBody>> SoftBodies;
	/// \brief The rigid bodies that are registered in this world
	std::set<std::shared_ptr<btRigidBody>> RigidBodies;
	/// \brief The constraints that are registered in this world
	std::set<std::shared_ptr<btTypedConstraint>> Constraints;

	std::unique_ptr<btBroadphaseInterface> Broadphase;
	std::unique_ptr<btCollisionConfiguration> CollisionConfiguration;
	std::unique_ptr<btCollisionDispatcher> CollisionDispatcher;
	std::unique_ptr<btConstraintSolver> ConstraintSolver;
	std::unique_ptr<btSoftRigidDynamicsWorld> DynamicsWorld;
	std::unique_ptr<btSoftBodySolver> SoftBodySolver;
};

}
