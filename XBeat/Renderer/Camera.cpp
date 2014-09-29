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

void XM_CALLCONV Camera::SetRotation(DirectX::XMFLOAT3& axis, float angle)
{
	m_rotation = DirectX::XMQuaternionRotationAxis(DirectX::XMLoadFloat3(&axis), angle);
}

void XM_CALLCONV Camera::SetRotation(DirectX::FXMVECTOR quaternion)
{
	m_rotation = quaternion;
}

DirectX::XMVECTOR Camera::GetRotation()
{
	return m_rotation;
}

bool Camera::Update(float msec)
{
	DirectX::XMVECTOR up, lookAt;

	up = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), m_rotation);
	lookAt = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), m_rotation);

	lookAt = DirectX::XMVectorAdd(m_position, lookAt);

	m_viewMatrix = DirectX::XMMatrixLookAtLH(m_position, lookAt, up);

	return true;
}

void XM_CALLCONV Camera::GetViewMatrix(DirectX::XMMATRIX &viewMatrix)
{
	viewMatrix = m_viewMatrix;
}
