#pragma once

#include "../DXUtil.h"
#include "../D3DRenderer.h"
#include "../D3DTextureRenderer.h"
#include "../OrthoWindowClass.h"
#include <memory>
#include <d3dx11effect.h>
#include <string>

namespace Renderer {
namespace Shaders {

#define BLUR_SAMPLE_COUNT 15

	class PostProcessEffect
{
public:
	struct ScreenSizeBuffer {
		DirectX::XMFLOAT2 dimentions;
		DirectX::XMFLOAT2 texelSize;
	};

	struct DOFBuffer {
		float nearZ;
		float farZ;
		float range;
		float unused;
	};

	struct BlurSamplersBuffer {
		DirectX::XMFLOAT4 offsetAndWeight[BLUR_SAMPLE_COUNT];
	};

	PostProcessEffect(void);
	~PostProcessEffect(void);

	bool Initialize(ID3D11Device *device, HWND wnd, int width, int height);
	void Shutdown();
	bool Render(std::shared_ptr<D3DRenderer> d3d, int indexCount, std::shared_ptr<D3DTextureRenderer> renderTexture, std::shared_ptr<OrthoWindowClass> window, float farZ, float nearZ);
	ID3D11Texture2D *GetCurrentOutput();
	ID3D11ShaderResourceView *GetCurrentOutputView();
private:
	bool InitializeEffect(ID3D11Device *device, HWND wnd, int width, int height, const std::wstring &filename);
	void ShutdownEffect();
	void OutputErrorMessage(ID3DBlob *error, HWND wnd, const std::wstring &filename);

	bool SetEffectParameters(ID3D11DeviceContext *context, float farZ, float nearZ);

	ID3DX11Effect *m_effect;
	ID3D11SamplerState *m_sampler;
	ID3D11InputLayout *m_layout;
	ID3D11Buffer *m_screenSizeBuffer;
	ID3D11Buffer *m_dofBuffer;
	ID3D11Buffer *m_blurBuffer;
	ID3D11Texture2D *m_originalBackTexture;
	ID3D11ShaderResourceView *m_originalBackView;
	ID3D11Texture2D *m_currentBackTexture;
	ID3D11ShaderResourceView *m_currentBackView;

	ID3DX11EffectVariable *m_originalSceneVar;
	ID3DX11EffectVariable *m_depthSceneVar;
	ID3DX11EffectVariable *m_currentSceneVar;
	ID3DX11EffectVariable *m_defaultSamplerVar;
	ID3DX11EffectConstantBuffer *m_screenSizeVar;
	ID3DX11EffectConstantBuffer *m_dofVar;
	ID3DX11EffectConstantBuffer *m_blurVar;

	uint32_t m_numGroups;
	uint32_t *m_numTechniques;
	uint32_t **m_numPasses;

	float m_blurAmmount;
};

}
}
