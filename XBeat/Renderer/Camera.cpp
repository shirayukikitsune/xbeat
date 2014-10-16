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

Camera::Camera(float FieldOfView, float AspectRatio, float NearPlane, float FarPlane)
	: FieldOfView(FieldOfView), AspectRatio(AspectRatio), NearPlane(NearPlane), FarPlane(FarPlane)
{
	m_rotation = DirectX::XMQuaternionIdentity();

	FocalDistance = 0.0f;
	Projection = DirectX::XMMatrixPerspectiveFovLH(FieldOfView, AspectRatio, NearPlane, FarPlane);
}

bool Camera::update(float Time)
{
	DirectX::XMVECTOR Up, LookAt;
	Up = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), m_rotation);
	LookAt = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), m_rotation) + m_position;
	DirectX::XMVECTOR EyePosition = DirectX::XMVectorScale(DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), m_rotation), FocalDistance) + m_position;

	ViewMatrix = DirectX::XMMatrixLookAtLH(EyePosition, LookAt, Up);

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

void Camera::setFocalDistance(float FocalDistance)
{
	this->FocalDistance = FocalDistance;
}
