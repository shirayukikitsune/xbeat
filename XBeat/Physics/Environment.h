#pragma once

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <memory>
#include <list>

namespace Renderer {
	class D3DRenderer;
}

namespace Physics {
struct PauseState {
	enum PauseState_e {
		Running,
		Paused,
		Holding
	};
};

class Environment
{
public:
	Environment(void);
	~Environment(void);

	bool Initialize();
	void Shutdown();
	bool Frame(float frameTimeMsec);

	__forceinline void Pause() { m_pauseState = PauseState::Paused; }
	__forceinline void Hold() { m_pauseState = PauseState::Holding; }
	void Unpause();
	__forceinline PauseState::PauseState_e GetPauseState() { return m_pauseState; }
	__forceinline bool IsRunning() { return m_pauseState == PauseState::Running; }
	__forceinline bool IsPaused() { return m_pauseState == PauseState::Paused; }
	__forceinline bool IsHolding() { return m_pauseState == PauseState::Holding; }
	__forceinline float GetPauseTime() { return m_pauseTime; }
	__forceinline btSoftBodyWorldInfo& GetWorldInfo() { return m_dynamicsWorld->getWorldInfo(); }

	void AddRigidBody(std::shared_ptr<btRigidBody> body, int16_t group = -1, int16_t mask = -1);
	void RemoveRigidBody(std::shared_ptr<btRigidBody> body);

	void AddCharacter(std::shared_ptr<btActionInterface> character);
	void RemoveCharacter(std::shared_ptr<btActionInterface> character);

private:
	PauseState::PauseState_e m_pauseState;
	float m_pauseTime;

	std::list<std::shared_ptr<btRigidBody>> m_rigidBodies;
	std::list<std::shared_ptr<btActionInterface>> m_characters;
	std::unique_ptr<btBroadphaseInterface> m_broadphase;
	std::unique_ptr<btCollisionConfiguration> m_collisionConfiguration;
	std::unique_ptr<btCollisionDispatcher> m_collisionDispatcher;
	std::unique_ptr<btConstraintSolver> m_constraintSolver;
	std::unique_ptr<btSoftRigidDynamicsWorld> m_dynamicsWorld;
	std::unique_ptr<btSoftBodySolver> m_softBodySolver;
};

}
