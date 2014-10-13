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
			: m_updatePhysics(true), m_position(DirectX::XMVectorZero()), m_rotation(DirectX::XMQuaternionIdentity())
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

		void XM_CALLCONV Move(float x, float y, float z) {
			Move(DirectX::XMVectorSet(x, y, z, 0.0f));
		}

		void XM_CALLCONV Move(DirectX::FXMVECTOR delta) {
			m_position = m_position + DirectX::XMVector3Rotate(delta, m_rotation);
		}

		DirectX::XMVECTOR XM_CALLCONV GetPosition() {
			return m_position;
		}

		void XM_CALLCONV SetRotation(float x, float y, float z, bool isRadians = false)
		{
			float yaw, pitch, roll;

			if (!isRadians) {
				pitch = DirectX::XMConvertToRadians(x);
				yaw = DirectX::XMConvertToRadians(y);
				roll = DirectX::XMConvertToRadians(z);
			}
			else {
				pitch = x;
				yaw = y;
				roll = z;
			}

			m_rotation = DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
		}

		void XM_CALLCONV SetRotation(DirectX::XMFLOAT3& axis, float angle)
		{
			m_rotation = DirectX::XMQuaternionRotationAxis(DirectX::XMLoadFloat3(&axis), angle);
		}

		void XM_CALLCONV SetRotation(DirectX::FXMVECTOR quaternion)
		{
			m_rotation = quaternion;
		}

		DirectX::XMVECTOR XM_CALLCONV GetRotation() {
			return m_rotation;
		}

	protected:
		std::shared_ptr<Shaders::Generic> m_shader;
		std::shared_ptr<Physics::Environment> m_physics;
		bool m_updatePhysics;
		DirectX::XMVECTOR m_position;
		DirectX::XMVECTOR m_rotation;
	};
}
