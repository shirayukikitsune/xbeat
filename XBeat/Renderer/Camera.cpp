#include "Camera.h"

using namespace Renderer;

Camera::Camera(void)
{
	m_rotation = DirectX::XMQuaternionIdentity();
}


void XM_CALLCONV Camera::SetRotation(float x, float y, float z)
{
	float yaw, pitch, roll;

	pitch = DirectX::XMConvertToRadians(x);
	yaw = DirectX::XMConvertToRadians(y);
	roll = DirectX::XMConvertToRadians(z);

	m_rotation = DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
}

void XM_CALLCONV Camera::SetRotation(btVector3& axis, float angle)
{
	m_rotation = DirectX::XMQuaternionRotationAxis(axis.get128(), angle);
}

void XM_CALLCONV Camera::SetRotation(btQuaternion &quaternion)
{
	m_rotation = quaternion.get128();
}

DirectX::XMVECTOR Camera::GetRotation()
{
	return m_rotation;
}

void Camera::Render()
{
	DirectX::XMVECTOR up, lookAt;

	up = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), m_rotation);
	lookAt = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), m_rotation);

	lookAt = DirectX::XMVectorAdd(m_position, lookAt);

	m_viewMatrix = DirectX::XMMatrixLookAtLH(m_position, lookAt, up);
}

void XM_CALLCONV Camera::GetViewMatrix(DirectX::XMMATRIX &viewMatrix)
{
	viewMatrix = m_viewMatrix;
}
