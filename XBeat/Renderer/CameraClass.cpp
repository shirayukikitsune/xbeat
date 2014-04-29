#include "CameraClass.h"

using namespace Renderer;

CameraClass::CameraClass(void)
{
	position = DirectX::XMVectorZero();
	rotation = DirectX::XMQuaternionIdentity();
}


CameraClass::~CameraClass(void)
{
}

void CameraClass::SetPosition(float x, float y, float z)
{
	position = DirectX::XMVectorSet(x, y, z, 0.0f);
}

void CameraClass::SetRotation(float x, float y, float z)
{
	float yaw, pitch, roll;

	pitch = DirectX::XMConvertToRadians(x);
	yaw = DirectX::XMConvertToRadians(y);
	roll = DirectX::XMConvertToRadians(z);

	rotation = DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
}

void CameraClass::SetRotation(btVector3& axis, float angle)
{
	rotation = DirectX::XMQuaternionRotationAxis(axis.get128(), angle);
}

void CameraClass::SetRotation(btQuaternion &quaternion)
{
	rotation = quaternion.get128();
}

DirectX::XMVECTOR CameraClass::GetPosition()
{
	return position;
}

DirectX::XMVECTOR CameraClass::GetRotation()
{
	return rotation;
}

void CameraClass::Render()
{
	DirectX::XMVECTOR up, lookAt;

	up = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation);
	lookAt = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);

	lookAt = DirectX::XMVectorAdd(position, lookAt);

	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, up);
}

void CameraClass::GetViewMatrix(DirectX::XMMATRIX &viewMatrix)
{
	viewMatrix = this->viewMatrix;
}
