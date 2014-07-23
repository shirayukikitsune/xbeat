#pragma once

// Include all commonly used Direct3D headers here
#include <D3D11.h>
#include <DirectXMath.h>
#include <utility>

#define DX_DELETEIF(v) if (v) { v->Release(); v = nullptr; }

namespace DirectX
{
	class XMTRANSFORM {
		XMMATRIX m_basis;

	public:
		XMTRANSFORM(const XMTRANSFORM &other)
		{
			this->m_basis = other.m_basis;
		}

		XMTRANSFORM(XMTRANSFORM && other)
		{
			this->m_basis = std::move(other.m_basis);
		}

		XMTRANSFORM(FXMVECTOR origin, FXMVECTOR rotation, FXMVECTOR translation)
		{
			this->m_basis = XMMatrixAffineTransformation(XMVectorSplatOne(), origin, rotation, translation);
		}

		XMTRANSFORM(const XMMATRIX &basis)
		{
			this->m_basis = basis;
		}

		XMTRANSFORM(XMMATRIX && basis)
		{
			this->m_basis = std::move(basis);
		}

		XMTRANSFORM()
		{
			this->m_basis = XMMatrixIdentity();
		}

		//! Implicit convertion to XMMATRIX
		operator XMMATRIX() {
			return m_basis;
		}
		//! Implicit convertion to XMMATRIX
		operator XMMATRIX() const {
			return m_basis;
		}

		//! Returns a new XMTRANSFORM that is the combinate of two XMTRANSFORM
		XMTRANSFORM operator * (const XMTRANSFORM &other) const
		{
			return this->m_basis * other.m_basis;
		}

		//! Rotate and translate a point/vector
		XMVECTOR XM_CALLCONV PointTransform(FXMVECTOR in)
		{
			// We set the W to 1.0, so the translation part of the matrix affects this vector
			XMVECTOR v = XMVectorSetW(in, 1.0f);

			return XMVector4Transform(v, this->m_basis);
		}

		//! Rotate a point/vector
		XMVECTOR XM_CALLCONV VectorTransform(FXMVECTOR in)
		{
			// We set the W to 0.0, so the translation part of the matrix doesn't affect this vector
			XMVECTOR v = XMVectorSetW(in, 0.0f);

			return XMVector4Transform(v, this->m_basis);
		}

#pragma region Append
		void XM_CALLCONV AppendTranslation(FXMVECTOR translation)
		{
			AppendTransform(XMMatrixTranslationFromVector(translation));
		}

		void XM_CALLCONV AppendRotation(FXMVECTOR quaternion)
		{
			AppendTransform(XMMatrixRotationQuaternion(quaternion));
		}

		void XM_CALLCONV AppendTransform(const XMTRANSFORM &other)
		{
			AppendTransform(other.m_basis);
		}

		void XM_CALLCONV AppendTransform(FXMMATRIX transform)
		{
			this->m_basis = XMMatrixMultiply(this->m_basis, transform);
		}
#pragma endregion

#pragma region Prepend
		void XM_CALLCONV PrependTranslation(FXMVECTOR translation)
		{
			PrependTransform(XMMatrixTranslationFromVector(translation));
		}

		void XM_CALLCONV PrependRotation(FXMVECTOR quaternion)
		{
			PrependTransform(XMMatrixRotationQuaternion(quaternion));
		}

		void XM_CALLCONV PrependTransform(const XMTRANSFORM &other)
		{
			PrependTransform(other.m_basis);
		}

		void XM_CALLCONV PrependTransform(FXMMATRIX transform)
		{
			this->m_basis = XMMatrixMultiply(transform, this->m_basis);
		}
#pragma endregion

		//! Returns the rotation as a quaternion
		XMVECTOR XM_CALLCONV GetRotationQuaternion()
		{
			return XMQuaternionRotationMatrix(this->m_basis);
		}

		//! Returns the rotation as a matrix
		XMMATRIX XM_CALLCONV GetRotationMatrix()
		{
			return XMMatrixRotationQuaternion(GetRotationQuaternion());
		}

		//! Returns the translation component
		XMVECTOR XM_CALLCONV GetTranslation()
		{
			return XMVectorSet(XMVectorGetW(this->m_basis.r[0]), XMVectorGetW(this->m_basis.r[1]), XMVectorGetW(this->m_basis.r[2]), 0.0f);
		}

		//! Returns the offset for rotation
		XMVECTOR XM_CALLCONV GetOffset()
		{
			return this->m_basis.r[3];
		}

		void XM_CALLCONV SetRotationQuaternion(FXMVECTOR in)
		{
			this->m_basis = XMMatrixAffineTransformation(XMVectorSplatOne(), GetOffset(), in, GetTranslation());
		}

		void XM_CALLCONV SetTranslation(FXMVECTOR in)
		{
			this->m_basis.r[0] = XMVectorSetW(this->m_basis.r[0], XMVectorGetX(in));
			this->m_basis.r[1] = XMVectorSetW(this->m_basis.r[1], XMVectorGetY(in));
			this->m_basis.r[2] = XMVectorSetW(this->m_basis.r[2], XMVectorGetZ(in));
		}

		void XM_CALLCONV SetOffset(FXMVECTOR in)
		{
			this->m_basis.r[3] = XMVectorSetW(in, 1.0f);
		}

		void XM_CALLCONV SetTransform(FXMMATRIX in)
		{
			this->m_basis = in;
		}
	};
}