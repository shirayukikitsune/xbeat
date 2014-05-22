#pragma once

#include <memory>
#include "ViewFrustum.h"
#include "D3DRenderer.h"
#include <DirectXMath.h>

namespace Physics {
	class Environment;
}

namespace Renderer {
	class BaseShader;

	class Entity
	{
	public:

		Entity()
			: m_updatePhysics(true), m_position(DirectX::XMVectorZero())
		{
		}

		virtual ~Entity()
		{
		}

		virtual bool Update(float msec) = 0;

		bool SetPhysicsSimulation(bool enabled) { m_updatePhysics = enabled; }
		void SetPhysics(std::shared_ptr<Physics::Environment> env) { m_physics = env; }

		std::shared_ptr<BaseShader> GetShader() { return m_shader; }
		void SetShader(std::shared_ptr<BaseShader> shader) { m_shader = shader; }

		virtual void Render(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<ViewFrustum> frustum) = 0;

		void XM_CALLCONV SetPosition(float x, float y, float z)
		{
			m_position = DirectX::XMVectorSet(x, y, z, 0.0f);
		}

		void XM_CALLCONV SetPosition(DirectX::XMVECTOR &position) {
			m_position = position;
		}

		void XM_CALLCONV GetPosition(DirectX::XMVECTOR &position) {
			position = m_position;
		}

	protected:
		std::shared_ptr<BaseShader> m_shader;
		std::shared_ptr<Physics::Environment> m_physics;
		bool m_updatePhysics;
		DirectX::XMVECTOR m_position;
	};
}
