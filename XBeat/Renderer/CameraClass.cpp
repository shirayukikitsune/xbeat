#include "CameraClass.h"

using namespace Renderer;

CameraClass::CameraClass(void)
{
	position = DirectX::XMVectorZero();
	rotation = DirectX::XMVectorZero();
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
	rotation = DirectX::XMVectorSet(x, y, z, 0.0f);
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
	float yaw, pitch, roll;
	DirectX::XMMATRIX rotationMatrix;

	up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	lookAt = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	pitch = DirectX::XMConvertToRadians(DirectX::XMVectorGetX(this->rotation));
	yaw = DirectX::XMConvertToRadians(DirectX::XMVectorGetY(this->rotation));
	roll = DirectX::XMConvertToRadians(DirectX::XMVectorGetZ(this->rotation));

	rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	lookAt = DirectX::XMVector3TransformCoord(lookAt, rotationMatrix);
	up = DirectX::XMVector3TransformCoord(up, rotationMatrix);

	lookAt = DirectX::XMVectorAdd(position, lookAt);

	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, up);
}

void CameraClass::GetViewMatrix(DirectX::XMMATRIX &viewMatrix)
{
	viewMatrix = this->viewMatrix;
}
