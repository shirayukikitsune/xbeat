#pragma once

// Include all commonly used Direct3D headers here
#include <D3D11.h>
#include <DirectXMath.h>
#include <utility>
#include "LinearMath/btTransform.h" // Convert XMTRANSFORM to/from btTransform

#define DX_DELETEIF(v) if (v) { v->Release(); v = nullptr; }

namespace DirectX
{
	class XMTRANSFORM {
		XMVECTOR m_translation;
		XMVECTOR m_rotation;
		XMVECTOR m_offset;

	public:
		XMTRANSFORM(const XMTRANSFORM &other)
		{
			this->m_translation = other.m_translation;
			this->m_rotation = other.m_rotation;
			this->m_offset = other.m_offset;
		}

		XMTRANSFORM(XMTRANSFORM && other)
		{
			this->m_translation = std::move(other.m_translation);
			this->m_rotation = std::move(other.m_rotation);
			this->m_offset = std::move(other.m_offset);
		}

		XMTRANSFORM(FXMVECTOR rotation, FXMVECTOR translation, FXMVECTOR offset)
		{
			this->m_translation = translation;
			this->m_rotation = rotation;
			this->m_offset = offset;
		}

		XMTRANSFORM(FXMMATRIX basis)
		{
			this->m_rotation = XMQuaternionRotationMatrix(basis);
			this->m_translation = basis.r[3];
			this->m_offset = XMVectorZero();
		}

		explicit XMTRANSFORM(const btTransform &t)
		{
			this->m_rotation = t.getRotation().get128();
			this->m_translation = t.getOrigin().get128();
			this->m_offset = XMVectorZero();
		}

		XMTRANSFORM()
		{
			this->m_rotation = XMQuaternionIdentity();
			this->m_translation = XMVectorZero();
			this->m_offset = XMVectorZero();
		}

		//! Implicit convertion to XMMATRIX
		operator XMMATRIX() {
			return XMMatrixAffineTransformation(XMVectorSplatOne(), m_offset, m_rotation, m_translation);
		}
		//! Implicit convertion to XMMATRIX
		operator XMMATRIX() const {
			return XMMatrixAffineTransformation(XMVectorSplatOne(), m_offset, m_rotation, m_translation);
		}

		//! Explicit convertion to btTransform
		explicit operator btTransform() {
			btQuaternion q;
			q.set128(m_rotation);
			btVector3 p;
			p.set128(m_translation);
			return btTransform(q, p);
		}

		//! Returns a new XMTRANSFORM that is the combinate of two XMTRANSFORM
		XMTRANSFORM operator * (const XMTRANSFORM &other) const
		{
			return XMTRANSFORM(XMQuaternionNormalize(XMQuaternionMultiply(this->m_rotation, other.m_rotation)), this->PointTransform(other.m_translation), this->PointTransform(other.m_offset));
		}

		//! Combine this transform with another one
		XMTRANSFORM operator *= (const XMTRANSFORM &other)
		{
			XMTRANSFORM t = *this * other;
			m_translation = t.m_translation;
			m_rotation = t.m_rotation;
			m_offset = t.m_offset;
			return *this;
		}

		//! Rotate and translate a point/vector
		XMVECTOR XM_CALLCONV PointTransform(FXMVECTOR in) const
		{
			return XMVectorAdd(VectorTransform(in), m_translation);
		}

		//! Rotate a point/vector
		XMVECTOR XM_CALLCONV VectorTransform(FXMVECTOR in) const
		{
			return XMVector3Rotate(in, m_rotation);
		}

#pragma region Append
		XMTRANSFORM& XM_CALLCONV AppendTranslation(FXMVECTOR translation)
		{
			m_translation = XMVectorAdd(m_translation, translation);
			return *this;
		}

		XMTRANSFORM& XM_CALLCONV AppendRotation(FXMVECTOR quaternion)
		{
			m_rotation = XMQuaternionNormalize(XMQuaternionMultiply(m_rotation, quaternion));
			return *this;
		}

		XMTRANSFORM& XM_CALLCONV AppendTransform(const XMTRANSFORM &other)
		{
			return AppendRotation(other.m_rotation).AppendTranslation(other.m_translation);
		}
#pragma endregion

#pragma region Prepend
		XMTRANSFORM& XM_CALLCONV PrependTranslation(FXMVECTOR translation)
		{
			m_translation = XMVectorAdd(translation, m_translation);
			return *this;
		}

		XMTRANSFORM& XM_CALLCONV PrependRotation(FXMVECTOR quaternion)
		{
			m_rotation = XMQuaternionNormalize(XMQuaternionMultiply(quaternion, m_rotation));
			return *this;
		}

		XMTRANSFORM& XM_CALLCONV PrependTransform(const XMTRANSFORM &other)
		{
			return PrependRotation(other.m_rotation).PrependTranslation(other.m_translation);
		}
#pragma endregion

		//! Returns the rotation as a quaternion
		XMVECTOR XM_CALLCONV GetRotationQuaternion()
		{
			return m_rotation;
		}

		//! Returns the translation component
		XMVECTOR XM_CALLCONV GetTranslation()
		{
			return m_translation;
		}

		//! Returns the offset component
		XMVECTOR XM_CALLCONV GetOffset()
		{
			return m_offset;
		}

		XMTRANSFORM& XM_CALLCONV SetRotationQuaternion(FXMVECTOR in)
		{
			m_rotation = in;
			return *this;
		}

		XMTRANSFORM& XM_CALLCONV SetTranslation(FXMVECTOR in)
		{
			m_translation = in;
			return *this;
		}

		XMTRANSFORM& XM_CALLCONV SetOffset(FXMVECTOR in)
		{
			m_offset = in;
			return *this;
		}

		XMTRANSFORM XM_CALLCONV Inverse()
		{
			return XMTRANSFORM(DirectX::XMQuaternionConjugate(m_rotation), DirectX::XMVectorNegate(m_translation), DirectX::XMVectorNegate(m_offset));
		}
	};
}