#pragma once

#include <memory>
#include "ViewFrustum.h"
#include "D3DRenderer.h"
#include <DirectXMath.h>

namespace Physics {
	class Environment;
}

namespace Renderer {
	namespace Shaders {
		class Generic;
	}

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

		virtual bool Update(float msec) { return true; }

		bool SetPhysicsSimulation(bool enabled) { m_updatePhysics = enabled; }
		void SetPhysics(std::shared_ptr<Physics::Environment> env) { m_physics = env; }

		std::shared_ptr<Shaders::Generic> GetShader() { return m_shader; }
		void SetShader(std::shared_ptr<Shaders::Generic> shader) { m_shader = shader; }

		virtual void Render(ID3D11DeviceContext *context, std::shared_ptr<ViewFrustum> frustum) {};

		void XM_CALLCONV SetPosition(float x, float y, float z)
		{
			m_position = DirectX::XMVectorSet(x, y, z, 0.0f);
		}

		void XM_CALLCONV SetPosition(DirectX::FXMVECTOR position) {
			m_position = position;
		}

		DirectX::XMVECTOR XM_CALLCONV GetPosition() {
			return m_position;
		}
		void XM_CALLCONV GetPosition(DirectX::XMVECTOR &position) {
			position = m_position;
		}

	protected:
		std::shared_ptr<Shaders::Generic> m_shader;
		std::shared_ptr<Physics::Environment> m_physics;
		bool m_updatePhysics;
		DirectX::XMVECTOR m_position;
	};
}
