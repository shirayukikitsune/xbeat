#include "MMDEffect.h"

#include <D3DX11async.h>
#include <D3Dcompiler.h>
#include <fstream>

using Renderer::Shaders::MMDEffect;


MMDEffect::MMDEffect(void)
{
}


MMDEffect::~MMDEffect(void)
{
	Shutdown();
}


bool MMDEffect::Initialize(ID3D11Device *device, HWND wnd, int width, int height)
{
	if (!InitializeEffect(device, wnd, width, height, L"./Data/Shaders/MMD.fx"))
		return false;

	return true;
}

void MMDEffect::Shutdown()
{
	ShutdownEffect();
}

bool MMDEffect::Render(std::shared_ptr<Renderer::D3DRenderer> d3d, int indexCount, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, std::shared_ptr<Renderer::D3DTextureRenderer> renderTexture, std::shared_ptr<Renderer::OrthoWindowClass> window, float farZ, float nearZ)
{
	ID3D11DeviceContext *context = d3d->GetDeviceContext();

	//RenderEffect(context, indexCount);

	ID3D11Resource *ptex;
	D3D11_MAPPED_SUBRESOURCE m;
	HRESULT r;
	bufferIndex = 0;
	renderTexture->CopyIntoTexture(context, &m_originalBackTexture, 0);
	renderTexture->CopyIntoTexture(context, &m_currentBackTexture, 0);
	context->CopyResource(m_originalDepthTexture, d3d->GetDepthStencilTexture());

	for (uint32_t group = 0; group < m_numGroups; group++) 
	{
		for (uint32_t technique = 0; technique < m_numTechniques[group]; technique++)
		{
			for (uint32_t pass = 0; pass < m_numPasses[group][technique]; pass++)
			{
				ID3D11ShaderResourceView *textures[3];
				textures[0] = m_originalBackView;
				textures[1] = m_originalDepthView;
				textures[2] = m_currentBackView;

				//bufferIndex++;
				renderTexture->SetRenderTarget(context, d3d->GetDepthStencilView(), bufferIndex & 1);
				//renderTexture->ClearRenderTarget(context, d3d->GetDepthStencilView(), bufferIndex & 1, 0.0f, 0.0f, 0.0f, 1.0f);

				d3d->Begin2D();

				window->Render(context);

				m_effect->GetGroupByIndex(group)->GetTechniqueByIndex(technique)->GetPassByIndex(pass)->Apply(0, context);

				context->IASetInputLayout(m_layout);
				context->PSSetSamplers(0, 1, &m_sampler);

				if (!SetEffectParameters(context, world, view, projection, textures, 3, farZ, nearZ))
					return false;

				context->DrawIndexed(indexCount, 0, 0);

				renderTexture->CopyIntoTexture(context, &m_currentBackTexture, bufferIndex & 1);

				d3d->End2D();
				d3d->SetBackBufferRenderTarget();
			}
		}
	}

	d3d->ResetViewport();

	return true;
}

bool MMDEffect::InitializeEffect(ID3D11Device *device, HWND wnd, int width, int height, const std::wstring &filename)
{
	HRESULT result;
	ID3D10Blob *bytecode, *errorMsg;
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_INPUT_ELEMENT_DESC layoutDesc[2];
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_TEXTURE2D_DESC texDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

	ZeroMemory(&texDesc, sizeof (D3D11_TEXTURE2D_DESC));

	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	result = device->CreateTexture2D(&texDesc, NULL, &m_originalBackTexture);
	if (FAILED(result))
		return false;

	result = device->CreateTexture2D(&texDesc, NULL, &m_currentBackTexture);
	if (FAILED(result))
		return false;

	shaderResourceViewDesc.Format = texDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(m_originalBackTexture, &shaderResourceViewDesc, &m_originalBackView);
	if (FAILED(result))
		return false;

	result = device->CreateShaderResourceView(m_currentBackTexture, &shaderResourceViewDesc, &m_currentBackView);
	if (FAILED(result))
		return false;

	shaderResourceViewDesc.Format = texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	result = device->CreateTexture2D(&texDesc, NULL, &m_originalDepthTexture);
	if (FAILED(result))
		return false;

	result = device->CreateShaderResourceView(m_originalDepthTexture, &shaderResourceViewDesc, &m_originalDepthView);
	if (FAILED(result))
		return false;

	result = D3DX11CompileFromFile(filename.c_str(), NULL, NULL, NULL, "fx_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, NULL, &bytecode, &errorMsg, NULL);
	if (FAILED(result)) {
		if (errorMsg == NULL) {
			MessageBox(wnd, (std::wstring(L"Shader file not found: ") + filename).c_str(), L"Error", MB_ICONERROR | MB_OK);
			return false;
		}
		else {
			OutputErrorMessage(errorMsg, wnd, filename);
			return false;
		}
	}

	result = D3DX11CreateEffectFromMemory(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), 0, device, &m_effect);
	bytecode->Release();
	if (FAILED(result)) {
		return false;
	}

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	for (auto &i : samplerDesc.BorderColor)
		i = 0.0f;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = device->CreateSamplerState(&samplerDesc, &m_sampler);
	if (FAILED(result))
		return false;

	// Create vertex shader input layout
	layoutDesc[0].SemanticName = "POSITION";
	layoutDesc[0].SemanticIndex = 0;
	layoutDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[0].InputSlot = 0;
	layoutDesc[0].AlignedByteOffset = 0;
	layoutDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[0].InstanceDataStepRate = 0;

	layoutDesc[1].SemanticName = "TEXCOORD";
	layoutDesc[1].SemanticIndex = 0;
	layoutDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	layoutDesc[1].InputSlot = 0;
	layoutDesc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	layoutDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[1].InstanceDataStepRate = 0;

	// Create matrix buffer
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.ByteWidth = sizeof (MatrixBuffer);
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);
	if (FAILED(result))
		return false;

	matrixBufferDesc.ByteWidth = sizeof (DOFBuffer);

	result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_dofBuffer);
	if (FAILED(result))
		return false;

	matrixBufferDesc.ByteWidth = sizeof (ScreenSizeBuffer);
	matrixBufferDesc.CPUAccessFlags = 0;
	matrixBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

	ScreenSizeBuffer ssb;
	ssb.dimentions = DirectX::XMFLOAT2(width, height);
	ssb.texelSize = DirectX::XMFLOAT2(texDesc.Width / ssb.dimentions.x, texDesc.Height / ssb.dimentions.y);
	D3D11_SUBRESOURCE_DATA srd;
	srd.pSysMem = (LPVOID)&ssb;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;
	result = device->CreateBuffer(&matrixBufferDesc, &srd, &m_screenSizeBuffer);
	if (FAILED(result))
		return false;

	// Parse the effect info
	D3DX11_GROUP_DESC groupDesc;
	D3DX11_EFFECT_DESC effectDesc;
	D3DX11_TECHNIQUE_DESC techniqueDesc;
	D3DX11_PASS_DESC passDesc;

	result = m_effect->GetDesc(&effectDesc);
	if (FAILED(result))
		return false;

	m_numGroups = effectDesc.Groups;
	m_numTechniques = new uint32_t[m_numGroups];
	m_numPasses = new uint32_t*[m_numGroups];

	ID3D11ShaderReflection *reflection;
	ID3DX11EffectGroup *group;
	ID3DX11EffectTechnique *technique;
	ID3DX11EffectPass *pass;

	for (uint32_t g = 0; g < m_numGroups; g++) {
		group = m_effect->GetGroupByIndex(g);
		if (group == NULL)
			return false;
		result = group->GetDesc(&groupDesc);
		if (FAILED(result))
			return false;

		m_numTechniques[g] = groupDesc.Techniques;
		m_numPasses[g] = new uint32_t[m_numTechniques[g]];

		for (uint32_t n = 0; n < m_numTechniques[g]; n++) {
			technique = group->GetTechniqueByIndex(n);
			if (technique == NULL)
				return false;
			result = technique->GetDesc(&techniqueDesc);
			if (FAILED(result))
				return false;

			m_numPasses[g][n] = techniqueDesc.Passes;

			// Only the first pass has a vertex shader
			pass = technique->GetPassByIndex(0);
			if (pass == NULL)
				return false;
			result = pass->GetDesc(&passDesc);
			if (FAILED(result))
				return false;
			result = device->CreateInputLayout(layoutDesc, 2, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_layout);
			if (FAILED(result))
				return false;
		}
	}

	return true;
}

void MMDEffect::ShutdownEffect()
{
	if (m_numPasses != nullptr) {
		delete[] m_numPasses;
		m_numPasses = nullptr;
	}

	if (m_matrixBuffer) {
		m_matrixBuffer = nullptr;
	}

	if (m_sampler) {
		m_sampler = nullptr;
	}

	if (m_layout) {
		m_layout = nullptr;
	}

	if (m_effect) {
		m_effect = nullptr;
	}
}

void MMDEffect::OutputErrorMessage(ID3D10Blob *errorMessage, HWND wnd, const std::wstring &file)
{
	char *compileError;
	SIZE_T bufferSize, i;
	std::ofstream fout;

	compileError = (char*)errorMessage->GetBufferPointer();
	bufferSize = errorMessage->GetBufferSize();

	fout.open("shader-errors.log", std::ios::app);
	if (fout.bad()) {
		errorMessage->Release();
		return;
	}

	fout.write(compileError, bufferSize);

	fout.close();

	errorMessage->Release();

	MessageBox(wnd, L"Error compiling effect", file.c_str(), MB_OK);
}

bool MMDEffect::SetEffectParameters(ID3D11DeviceContext *context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, ID3D11ShaderResourceView **textures, int textureCount, float farZ, float nearZ)
{
	MatrixBuffer *matrixBuffer;
	DOFBuffer *dofBuffer;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result;

	result = context->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result)) {
		return false;
	}

	matrixBuffer = (MatrixBuffer*)mappedResource.pData;

	matrixBuffer->world = world;
	matrixBuffer->view = view;
	matrixBuffer->projection = projection;

	context->Unmap(m_matrixBuffer, 0);

	result = context->Map(m_dofBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result)) {
		return false;
	}

	dofBuffer = (DOFBuffer*)mappedResource.pData;

	dofBuffer->unused = 0.0f;
	dofBuffer->range = 0.05f;
	dofBuffer->nearZ = nearZ;
	dofBuffer->farZ = farZ;

	context->Unmap(m_dofBuffer, 0);

	context->VSSetConstantBuffers(0, 1, &m_matrixBuffer);
	context->VSSetConstantBuffers(1, 1, &m_screenSizeBuffer); 

	context->PSSetShaderResources(0, textureCount, textures);
	context->PSSetConstantBuffers(0, 1, &m_dofBuffer);

	return true;
}

void MMDEffect::RenderEffect(ID3D11DeviceContext *context, int indexCount)
{
}

Renderer::DXType<ID3D11Texture2D> MMDEffect::GetCurrentOutput()
{
	return m_currentBackTexture;
}

Renderer::DXType<ID3D11ShaderResourceView> MMDEffect::GetCurrentOutputView()
{
	return m_currentBackView;
}
