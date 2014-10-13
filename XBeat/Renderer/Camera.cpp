//===-- Renderer/Camera.cpp - Defines the class for controlling the camera --*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines the Renderer::Camera class
///
//===----------------------------------------------------------------------------------===//

#include "Camera.h"

using namespace Renderer;

Camera::Camera(void)
{
	m_rotation = DirectX::XMQuaternionIdentity();
}

bool Camera::update(float Time)
{
	DirectX::XMVECTOR up, lookAt;

	up = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), m_rotation);
	lookAt = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), m_rotation);

	lookAt = DirectX::XMVectorAdd(m_position, lookAt);

	ViewMatrix = DirectX::XMMatrixLookAtLH(m_position, lookAt, up);

	return true;
}

void XM_CALLCONV Camera::getViewMatrix(DirectX::XMMATRIX &Matrix)
{
	Matrix = ViewMatrix;
}

void XM_CALLCONV Camera::getProjectionMatrix(DirectX::XMMATRIX &Matrix)
{
	Matrix = Projection;
}

void Camera::setFieldOfView(float FieldOfView)
{
	this->FieldOfView = FieldOfView;
	Projection = DirectX::XMMatrixPerspectiveFovLH(FieldOfView, AspectRatio, NearPlane, FarPlane);
}

void Camera::setAspectRatio(float AspectRatio)
{
	this->AspectRatio = AspectRatio;
	Projection = DirectX::XMMatrixPerspectiveFovLH(FieldOfView, AspectRatio, NearPlane, FarPlane);
}

void Camera::setNearPlane(float NearPlane)
{
	this->NearPlane = NearPlane;
	Projection = DirectX::XMMatrixPerspectiveFovLH(FieldOfView, AspectRatio, NearPlane, FarPlane);
}

void Camera::setFarPlane(float FarPlane)
{
	this->FarPlane = FarPlane;
	Projection = DirectX::XMMatrixPerspectiveFovLH(FieldOfView, AspectRatio, NearPlane, FarPlane);
}
