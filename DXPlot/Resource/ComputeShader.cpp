#include "Resource/ComputeShader.hpp"
#include "util.hpp"

#include <exception>

using namespace Cass;

ComputeShader::ComputeShader(LPCWSTR _fileName, LPCSTR _entryPoint, ID3D11Device*& _pDevice): fileName(_fileName), entryPoint(_entryPoint) {
	if (!_pDevice || !entryPoint || !fileName)
		throw std::invalid_argument("Inavlid arguments passed to contructor");

	HRESULT hr = E_FAIL;
	LPCSTR profile = _pDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ? "cs_5_0" : "cs_4_0";

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* shader;
	ID3DBlob* error;
	hr = D3DCompileFromFile(fileName, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, profile, flags, NULL, &shader, &error);
	
	if (FAILED(hr)) {
		if (error) {
			OutputDebugStringA(reinterpret_cast <LPCSTR> (error->GetBufferPointer()));
			error->Release();
		}

		if (shader) shader->Release();
		ThrowIfFailed(hr);
	}

	hr = _pDevice->CreateComputeShader(shader->GetBufferPointer(), shader->GetBufferSize(), nullptr, &m_computeShader);
	ThrowIfFailed(hr);
}

void ComputeShader::CreateSRVFromTexture(DXGI_FORMAT format, D3D11_SRV_DIMENSION dimension, ID3D11Device*& _pDevice, ID3D11Texture2D*& _pResource) {
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	ZeroMemory(&srvd, sizeof(srvd));
	srvd.Format = format;
	srvd.ViewDimension = dimension;

	ThrowIfFailed(_pDevice->CreateShaderResourceView(_pResource, &srvd, &m_SRV));
}

void ComputeShader::CreateUAVFromTexture(DXGI_FORMAT format, D3D11_UAV_DIMENSION dimension, ID3D11Device*& _pDevice, ID3D11Texture2D*& _pResource) {
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
	ZeroMemory(&uavd, sizeof(uavd));
	uavd.Format = format;
	uavd.ViewDimension = dimension;

	ThrowIfFailed(_pDevice->CreateUnorderedAccessView(_pResource, &uavd, &m_UAV));
}

void ComputeShader::Execute(UINT groupCountX, UINT groupCountY, ID3D11DeviceContext*& _pContext) {
	_pContext->CSSetShader(m_computeShader.Get(), nullptr, NULL);
	_pContext->CSSetShaderResources(0, 1, m_SRV.GetAddressOf());
	_pContext->CSSetUnorderedAccessViews(0, 1, m_UAV.GetAddressOf(), NULL);

	_pContext->Dispatch(groupCountX, groupCountY, 1);

	ID3D11ShaderResourceView* ppNullSRV[2] = { NULL, NULL };
	ID3D11UnorderedAccessView* ppNullUAV[1] = { NULL };

	_pContext->CSSetShader(NULL, NULL, NULL);
	_pContext->CSSetShaderResources(0, 2, ppNullSRV);
	_pContext->CSSetUnorderedAccessViews(0, 1, ppNullUAV, NULL);
}