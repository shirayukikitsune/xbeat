#include "Camera.h"

using namespace Renderer;

Camera::Camera(void)
{
	m_rotation = DirectX::XMQuaternionIdentity();
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
