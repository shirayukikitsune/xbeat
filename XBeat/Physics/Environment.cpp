#include "../Renderer/D3DRenderer.h"
#include "Environment.h"

#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletMultiThreaded/GpuSoftBodySolvers/DX11/btSoftBodySolver_DX11.h>

#include <algorithm>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3dx11.lib")

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

bool Environment::Initialize(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	m_broadphase.reset(new btDbvtBroadphase);
	if (!m_broadphase)
		return false;

	m_collisionConfiguration.reset(new btSoftBodyRigidBodyCollisionConfiguration);
	if (!m_collisionConfiguration)
		return false;

	m_collisionDispatcher.reset(new btCollisionDispatcher(m_collisionConfiguration.get()));
	if (!m_collisionDispatcher)
		return false;

	m_constraintSolver.reset(new btSequentialImpulseConstraintSolver);
	if (!m_constraintSolver)
		return false;

	if (!d3d)
		m_softBodySolver.reset(new btDefaultSoftBodySolver);
	else
		m_softBodySolver.reset(new btDX11SoftBodySolver(d3d->GetDevice(), d3d->GetDeviceContext()));
	if (!m_softBodySolver)
		return false;

	//m_softBodySolverOutput.reset(new btSoftBodySolverOutputDXtoDX(d3d->GetDevice(), d3d->GetDeviceContext()));
	m_softBodySolverOutput.reset(new btSoftBodySolverOutputDXtoCPU);
	if (!m_softBodySolverOutput)
		return false;

	m_dynamicsWorld.reset(new btSoftRigidDynamicsWorld(m_collisionDispatcher.get(), m_broadphase.get(), m_constraintSolver.get(), m_collisionConfiguration.get(), m_softBodySolver.get()));
	if (!m_dynamicsWorld)
		return false;

	m_dynamicsWorld->setGravity(btVector3(0, -10, 0));

	return true;
}

void Environment::Shutdown()
{
	for (auto i = m_rigidBodies.begin(); !m_rigidBodies.empty(); i = m_rigidBodies.begin()) {
		m_dynamicsWorld->removeRigidBody(i->get());
		m_rigidBodies.erase(i);
	}
	if (m_dynamicsWorld != nullptr)
		m_dynamicsWorld.reset();
	if (m_softBodySolverOutput != nullptr)
		m_softBodySolverOutput.reset();
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

void Environment::AddSoftBody(std::shared_ptr<btSoftBody> body, int16_t group, int16_t mask)
{
	m_softBodies.insert(body);
	m_dynamicsWorld->addSoftBody(body.get(), group, mask);
}

void Environment::RemoveSoftBody(std::shared_ptr<btSoftBody> body)
{
	auto i = m_softBodies.find(body);

	if (i != m_softBodies.end()) {
		m_softBodies.erase(i);
		m_dynamicsWorld->removeSoftBody(body.get());
	}
}

void Environment::AddRigidBody(std::shared_ptr<btRigidBody> body, int16_t group, int16_t mask)
{
	m_rigidBodies.insert(body);
	m_dynamicsWorld->addRigidBody(body.get(), group, mask);
}

void Environment::RemoveRigidBody(std::shared_ptr<btRigidBody> body)
{
	auto i = m_rigidBodies.find(body);

	if (i != m_rigidBodies.end()) {
		m_rigidBodies.erase(i);
		m_dynamicsWorld->removeRigidBody(body.get());
	}
}

void Environment::AddCharacter(std::shared_ptr<btActionInterface> character)
{
	m_characters.insert(character);
	m_dynamicsWorld->addCharacter(character.get());
}

void Environment::RemoveCharacter(std::shared_ptr<btActionInterface> character)
{
	auto i = m_characters.find(character);

	if (i == m_characters.end()) {
		m_characters.erase(i);
		m_dynamicsWorld->removeCharacter(character.get());
	}
}
