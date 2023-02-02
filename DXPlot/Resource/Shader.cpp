#include "Resource/Shader.hpp"

#include <DirectXMath.h>

// ---------- class Shader

DX::Shader::Shader(DirectX::XMFLOAT4 _color) : m_color(_color) {}

namespace {
	// structs used for constant buffers must be a aligned to 16 bytes

	__declspec(align(16)) struct SURF_CBUFFERDATA_VS {
		DirectX::XMMATRIX modelMat;
		DirectX::XMMATRIX viewMat;
		DirectX::XMMATRIX projectionMat;
		DirectX::XMMATRIX normalMat;
	};

	__declspec(align(16)) struct SURF_CBUFFERDATA_PS {
		float worldScale;
		DirectX::XMVECTOR color;
		float roughness;
		float metallic;
	};

	__declspec(align(16)) struct FLAT_CBUFFERDATA_VS {
		DirectX::XMMATRIX modelMat;
		DirectX::XMMATRIX viewMat;
		DirectX::XMMATRIX projectionMat;
	};

	_declspec(align(16)) struct FLAT_CBUFFERDATA_PS {
		float worldScale;
		DirectX::XMVECTOR color;
	};
}

void DX::Shader::SetActive(ID3D11DeviceContext* _pContext, DX::Camera& camera, DirectX::XMMATRIX _modelMat, UINT32 flags) {
	if (_pContext == nullptr) return;

	SetBuffers(_pContext, camera, _modelMat, flags);

	_pContext->IASetInputLayout(m_inputLayout.Get());
	_pContext->VSSetShader(m_vertShader.Get(), nullptr, NULL);
	_pContext->PSSetShader(m_fragShader.Get(), nullptr, NULL);
}

HRESULT DX::Shader::CompileAndSetLayout(LPCWSTR fName, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, D3D11_INPUT_ELEMENT_DESC* ied, UINT numElements) {
	if (_pDevice == nullptr) return E_FAIL;

	HRESULT hr = S_OK;
	LPCSTR targVert = "vs_4_0", targFrag = "ps_4_0";
	if (_pDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) {
		targVert = "vs_5_0";
		targFrag = "ps_5_0";
	}

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG;
#endif

	Microsoft::WRL::ComPtr <ID3DBlob> error;

	// compile the shaders
	ID3DBlob* tp_bVert, * tp_bFrag;
	hr = D3DCompileFromFile(fName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vertex", targVert, flags, NULL, &tp_bVert, error.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		if (error) {
			OutputDebugStringA(reinterpret_cast <LPCSTR> (error->GetBufferPointer()));
			error.Reset();
		}
		return hr;
	}

	hr = D3DCompileFromFile(fName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "fragment", targFrag, flags, NULL, &tp_bFrag, error.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		if (error) {
			OutputDebugStringA(reinterpret_cast <LPCSTR> (error->GetBufferPointer()));
			error.Reset();
		}
		return hr;
	}

	// encapsulate shaders into shader objects
	hr = _pDevice->CreateVertexShader(tp_bVert->GetBufferPointer(), tp_bVert->GetBufferSize(), nullptr, m_vertShader.ReleaseAndGetAddressOf());	if (FAILED(hr)) return hr;
	hr = _pDevice->CreatePixelShader(tp_bFrag->GetBufferPointer(), tp_bFrag->GetBufferSize(), nullptr, m_fragShader.ReleaseAndGetAddressOf());	if (FAILED(hr)) return hr;

	hr = _pDevice->CreateInputLayout(ied, numElements, tp_bVert->GetBufferPointer(), tp_bVert->GetBufferSize(), m_inputLayout.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return hr;

	tp_bVert->Release();
	tp_bFrag->Release();

	return hr;
}


// ---------- class SurfaceShader

DX::SurfaceShader::SurfaceShader(std::shared_ptr <DX::Texture> _albedo, DirectX::XMFLOAT4 _color) : Shader(_color) {
	m_roughness = 0.5;
	m_metallic = 0.0f;
	m_albedo = _albedo;
}

HRESULT DX::SurfaceShader::LoadFromFile(LPCWSTR _fName, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext) {
	HRESULT hr = S_OK;

	// multiple datatypes can bind to a single slot of input layout, useful when the vertex buffer is a structure
	// (semanticName, semanticIndex, format, inputSlot, alignedByteOffset, InputSlotClass, InstanceDataPerStepRate)
	D3D11_INPUT_ELEMENT_DESC* ied = new D3D11_INPUT_ELEMENT_DESC[3] {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = CompileAndSetLayout(_fName, _pDevice, _pContext, ied, 3U);
	if (FAILED(hr)) return hr;

	// create constant buffers for both stages

	D3D11_BUFFER_DESC bd1;
	ZeroMemory(&bd1, sizeof(bd1));
	bd1.ByteWidth = sizeof(SURF_CBUFFERDATA_VS);
	bd1.Usage = D3D11_USAGE_DYNAMIC;
	bd1.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd1.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_BUFFER_DESC bd2;
	bd2 = bd1;
	bd2.ByteWidth = sizeof(SURF_CBUFFERDATA_PS);

	hr = _pDevice->CreateBuffer(&bd1, nullptr, m_vsCbuffer.ReleaseAndGetAddressOf());	if (FAILED(hr)) return hr;
	hr = _pDevice->CreateBuffer(&bd2, nullptr, m_psCbuffer.ReleaseAndGetAddressOf()); if (FAILED(hr)) return hr;

	delete[] ied;

	return hr;
}

void DX::SurfaceShader::SetBuffers(ID3D11DeviceContext* _pContext, DX::Camera& camera, DirectX::XMMATRIX _modelMat, uint32_t flags) {
	SURF_CBUFFERDATA_VS cb;
	ZeroMemory(&cb, sizeof(cb));
	cb.modelMat = _modelMat;
	cb.projectionMat = camera.GetProjectionMat();
	cb.viewMat = camera.GetViewMat();
	cb.normalMat = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(
		nullptr, 
		DirectX::XMMatrixMultiply(_modelMat, camera.GetViewMat())
	));

	SURF_CBUFFERDATA_PS cb1;
	ZeroMemory(&cb1, sizeof(cb1));
	cb1.color = DirectX::XMLoadFloat4(&m_color);
	cb1.metallic = m_metallic;
	cb1.roughness = m_roughness;
	cb1.worldScale = camera.GetScale().x;

	D3D11_MAPPED_SUBRESOURCE ms1;
	ZeroMemory(&ms1, sizeof(ms1));
	DX::ThrowIfFailed(_pContext->Map(m_vsCbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &ms1));
	memcpy(ms1.pData, &cb, sizeof(cb));
	_pContext->Unmap(m_vsCbuffer.Get(), 0);

	D3D11_MAPPED_SUBRESOURCE ms2;
	ZeroMemory(&ms2, sizeof(ms2));
	DX::ThrowIfFailed(_pContext->Map(m_psCbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &ms2));
	memcpy(ms2.pData, &cb1, sizeof(cb1));
	_pContext->Unmap(m_psCbuffer.Get(), 0);

	_pContext->VSSetConstantBuffers(0, 1, m_vsCbuffer.GetAddressOf());
	_pContext->PSSetConstantBuffers(0, 1, m_psCbuffer.GetAddressOf());

	if (m_albedo.get() != nullptr) {
		auto srv = m_albedo->GetSRV();
		auto sampler = m_albedo->GetSampler();
		_pContext->PSSetShaderResources(0, 1, &srv);
		_pContext->PSSetSamplers(0, 1, &sampler);
	}
}

// --------- class FlatShader

DX::FlatShader::FlatShader(DirectX::XMFLOAT4 _color) :Shader(_color) {}

HRESULT DX::FlatShader::LoadFromFile(LPCWSTR _fName, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext) {
	HRESULT hr = S_OK;

	D3D11_INPUT_ELEMENT_DESC* ied = new D3D11_INPUT_ELEMENT_DESC[2]{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = CompileAndSetLayout(_fName, _pDevice, _pContext, ied, 2);
	if (FAILED(hr)) return hr;

	D3D11_BUFFER_DESC bd1;
	ZeroMemory(&bd1, sizeof(bd1));
	bd1.ByteWidth = sizeof(FLAT_CBUFFERDATA_VS);
	bd1.Usage = D3D11_USAGE_DYNAMIC;
	bd1.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd1.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_BUFFER_DESC bd2;
	bd2 = bd1;
	bd2.ByteWidth = sizeof(FLAT_CBUFFERDATA_PS);

	hr = _pDevice->CreateBuffer(&bd1, nullptr, m_vsCbuffer.ReleaseAndGetAddressOf());	if (FAILED(hr)) return hr;
	hr = _pDevice->CreateBuffer(&bd2, nullptr, m_psCbuffer.ReleaseAndGetAddressOf());	if (FAILED(hr)) return hr;

	return hr;
}

void DX::FlatShader::SetBuffers(ID3D11DeviceContext* _pContext, DX::Camera& camera, DirectX::XMMATRIX _modelMat, uint32_t flags) {
	FLAT_CBUFFERDATA_VS cb1;
	ZeroMemory(&cb1, sizeof(cb1));
	cb1.modelMat = _modelMat;
	if (flags & SHADER_FLAGS_FLAT_NO_PROJECT) {
		cb1.viewMat = DirectX::XMMatrixIdentity();
		cb1.projectionMat = DirectX::XMMatrixIdentity();
	}
	else {
		cb1.viewMat = camera.GetViewMat();
		cb1.projectionMat = camera.GetProjectionMat();
	}

	FLAT_CBUFFERDATA_PS cb2;
	ZeroMemory(&cb2, sizeof(cb2));
	cb2.worldScale = camera.GetScale().x;
	cb2.color = DirectX::XMLoadFloat4(&m_color);

	D3D11_MAPPED_SUBRESOURCE ms1;
	_pContext->Map(m_vsCbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &ms1);
	memcpy(ms1.pData, &cb1, sizeof(cb1));
	_pContext->Unmap(m_vsCbuffer.Get(), 0);

	D3D11_MAPPED_SUBRESOURCE ms2;
	_pContext->Map(m_psCbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &ms2);
	memcpy(ms2.pData, &cb2, sizeof(cb2));
	_pContext->Unmap(m_psCbuffer.Get(), 0);

	_pContext->VSSetConstantBuffers(0, 1, m_vsCbuffer.GetAddressOf());
	_pContext->PSSetConstantBuffers(0, 1, m_psCbuffer.GetAddressOf());
}