#include "Environment.h"
#include "../Renderer/D3DRenderer.h"

#include <BulletSoftBody/btDefaultSoftBodySolver.h>

using Physics::Environment;

Environment::Environment(void)
{
	m_pauseState = PauseState::Running;
	m_pauseTime = 0.0f;
}


Environment::~Environment(void)
{
	Shutdown();
}

bool Environment::Initialize()
{
	m_broadphase.reset(new btDbvtBroadphase);
	if (!m_broadphase)
		return false;

	m_collisionConfiguration.reset(new btDefaultCollisionConfiguration);
	if (!m_collisionConfiguration)
		return false;

	m_collisionDispatcher.reset(new btCollisionDispatcher(m_collisionConfiguration.get()));
	if (!m_collisionDispatcher)
		return false;

	m_constraintSolver.reset(new btSequentialImpulseConstraintSolver);
	if (!m_constraintSolver)
		return false;

	m_softBodySolver.reset(new btDefaultSoftBodySolver);
	if (!m_softBodySolver)
		return false;

	m_dynamicsWorld.reset(new btSoftRigidDynamicsWorld(m_collisionDispatcher.get(), m_broadphase.get(), m_constraintSolver.get(), m_collisionConfiguration.get(), m_softBodySolver.get()));
	if (!m_dynamicsWorld)
		return false;

	m_dynamicsWorld->setGravity(btVector3(0, -10, 0));

	return true;
}

void Environment::Shutdown()
{
	while (!m_rigidBodies.empty()) {
		m_dynamicsWorld->removeRigidBody(m_rigidBodies.front().get());
		m_rigidBodies.pop_front();
	}
	if (m_dynamicsWorld != nullptr)
		m_dynamicsWorld.reset();
	if (m_softBodySolver != nullptr)
		m_softBodySolver.reset();
	if (m_constraintSolver != nullptr)
		m_constraintSolver.reset();
	if (m_collisionDispatcher != nullptr)
		m_collisionDispatcher.reset();
	if (m_collisionConfiguration != nullptr)
		m_collisionConfiguration.reset();
	if (m_broadphase != nullptr)
		m_broadphase.reset();
}

bool Environment::Frame(float frameTimeMsec)
{
	if (!IsRunning()) {
		// If we are on hold, increase the time counter, so when we resume, we can skip the simulation for this long
		if (IsHolding())
			m_pauseTime += frameTimeMsec;
		return true;
	}

	// Check if the screen was frozen
	if (m_pauseTime > 0.0f) {
		// If we were on hold, do all simulation while we were frozen on a single step
		m_dynamicsWorld->stepSimulation(m_pauseTime / 1000.f, 1, m_pauseTime / 1000.f);

		// We must not skip the current frame simulation, so dont exit yet!
	}

	// Do our frame simulation
	m_dynamicsWorld->stepSimulation(frameTimeMsec / 1000.f);

	return true;
}

void Environment::Unpause()
{
	m_pauseState = PauseState::Running;
}

void Environment::AddRigidBody(std::shared_ptr<btRigidBody> body, int16_t group, int16_t mask)
{
	m_rigidBodies.push_front(body);
	m_dynamicsWorld->addRigidBody(body.get(), group, mask);
}

void Environment::RemoveRigidBody(std::shared_ptr<btRigidBody> body)
{
	for (auto i = m_rigidBodies.begin(); i != m_rigidBodies.end(); i++)
	{
		if (i->get() == body.get()) {
			m_rigidBodies.erase(i);
			m_dynamicsWorld->removeRigidBody(body.get());
			return;
		}
	}
}

void Environment::AddCharacter(std::shared_ptr<btActionInterface> character)
{
	m_characters.push_front(character);
	m_dynamicsWorld->addCharacter(character.get());
}

void Environment::RemoveCharacter(std::shared_ptr<btActionInterface> character)
{
	for (auto i = m_characters.begin(); i != m_characters.end(); i++)
	{
		if (i->get() == character.get()) {
			m_characters.erase(i);
			m_dynamicsWorld->removeCharacter(character.get());
			return;
		}
	}
}

