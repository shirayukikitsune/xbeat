#pragma once

#include "../DXUtil.h"
#include <memory>
#include <string>

namespace Renderer {
	class Entity;

namespace Shaders {
	class Generic
	{
	public:
		struct ConstBuffer {
			struct Matrices {
				DirectX::XMMATRIX world;
				DirectX::XMMATRIX view;
				DirectX::XMMATRIX projection;
				DirectX::XMMATRIX wvp;
				DirectX::XMMATRIX worldInverse;
			} matrix;

			struct Light {
				DirectX::XMVECTOR ambient;
				DirectX::XMVECTOR diffuse;
				DirectX::XMVECTOR specularColorAndPower;
				DirectX::XMVECTOR direction;
				DirectX::XMVECTOR position;
			} lights[3];

			DirectX::XMVECTOR eyePosition;

			UINT lightCount = 0;
			UINT _padding[3];
		};

		Generic()
		{
		}

		virtual ~Generic()
		{
			Shutdown();
		}

		void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);
		void XM_CALLCONV SetEyePosition(DirectX::FXMVECTOR eyePosition);
		void XM_CALLCONV SetLights(DirectX::FXMVECTOR ambient, DirectX::FXMVECTOR diffuse, DirectX::FXMVECTOR specular, DirectX::GXMVECTOR direction, DirectX::HXMVECTOR position, int index);
		void XM_CALLCONV SetLightCount(UINT count);

		bool InitializeBuffers(ID3D11Device *device, HWND hwnd);

		bool Update(float msec, ID3D11DeviceContext *context);
		bool Render(ID3D11DeviceContext *context, UINT indexCount, UINT offset);

		void Shutdown();

		const ConstBuffer& GetCBuffer() const { return m_cbuffer; }

		static char* getFileContents(const std::wstring &file, SIZE_T &bufSize);

		void PrepareRender(ID3D11DeviceContext *context);
	private:
		ConstBuffer m_cbuffer;

	protected:
		virtual bool InternalInitializeBuffers(ID3D11Device *device, HWND hwnd) { return true; }
		virtual bool InternalUpdate(float msec, ID3D11DeviceContext *context) { return true; }
		virtual void InternalPrepareRender(ID3D11DeviceContext *context) {}
		virtual bool InternalRender(ID3D11DeviceContext *context, UINT indexCount, UINT offset) { return true; }
		virtual void InternalShutdown() {}

		void OutputShaderErrorMessage(ID3DBlob *errorMessage, HWND wnd, const std::wstring &file);

		ID3D11Buffer *m_dxcbuffer;
		ID3D11InputLayout *m_layout;
		ID3D11SamplerState *m_sampleState;
	};
}
}