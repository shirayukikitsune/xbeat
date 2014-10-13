//===-- Renderer/Camera.h - Declares the class for controlling the camera --*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares the Renderer::Camera class
///
//===---------------------------------------------------------------------------------===//

#pragma once

#include "Entity.h"

namespace Renderer {

	/// \brief Defines a camera to be used by the renderer
	class Camera
		: public Entity
	{
	public:
		Camera(float FieldOfView, float AspectRatio, float NearPlane, float FarPlane);

		/// \brief Updates the view matrix
		virtual bool update(float Time);

		/// \brief Returns the view matrix
		void XM_CALLCONV getViewMatrix(DirectX::XMMATRIX &Matrix);
		/// \brief Returns the projection matrix
		void XM_CALLCONV getProjectionMatrix(DirectX::XMMATRIX &Matrix);

		/// \brief Sets the field of view vertical angle
		///
		/// \param [in] FieldOfView The new field of view vertical angle, in radians
		void setFieldOfView(float FieldOfView);
		/// \brief Sets the projection aspect ratio
		void setAspectRatio(float AspectRatio);
		/// \brief Sets the near clipping plane distance
		void setNearPlane(float NearPlane);
		/// \brief Sets the far clipping plane distance
		void setFarPlane(float FarPlane);

		/// \brief Sets the focal distance
		void setFocalDistance(float FocalDistance);
		/// \brief Gets the focal distance
		float getFocalDistance() { return FocalDistance; }

	private:
		/// \brief The view matrix
		DirectX::XMMATRIX ViewMatrix;
		/// \brief The projection matrix
		DirectX::XMMATRIX Projection;

		/// \brief The distance to the focal point
		///
		/// \remarks The Entity::GetPosition() is the focal point
		float FocalDistance;

		/// \brief The field of view angle, in radians
		float FieldOfView;
		/// \brief The screen aspect ratio
		float AspectRatio;
		/// \brief The near clipping plane distance
		float NearPlane;
		/// \brief The far clipping plane distance
		float FarPlane;
	};

}
