#pragma once

#include "util.hpp"
#include "Object/Camera.hpp"
#include "Resource/Texture.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace DX {
	enum SHADER_FLAGS: uint32_t {
		SHADER_FLAGS_NONE = 0,
		SHADER_FLAGS_FLAT_NO_PROJECT = 1
	};

	class Shader {
	public:
		Shader(DirectX::XMFLOAT4 _color);
		void SetActive(ID3D11DeviceContext* _pContext, DX::Camera& camera, DirectX::XMMATRIX _modelMat, UINT32 flags = 0);

		DirectX::XMFLOAT4 m_color;
	
	protected:
		/**
		* @brief Compile Shader From File and set its input layout
		*/
		virtual HRESULT CompileAndSetLayout(LPCWSTR fName, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, D3D11_INPUT_ELEMENT_DESC* ied, UINT numElements);

		/**
		* @brief Set Constant Buffers for shader stages
		*/
		virtual void SetBuffers(ID3D11DeviceContext* _pContext, DX::Camera& camera, DirectX::XMMATRIX _modelMat, uint32_t flags = 0) = 0;
		virtual HRESULT LoadFromFile(LPCWSTR fName, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext) = 0;
		
		Microsoft::WRL::ComPtr <ID3D11VertexShader> m_vertShader;
		Microsoft::WRL::ComPtr <ID3D11PixelShader>	m_fragShader;
		Microsoft::WRL::ComPtr <ID3D11InputLayout>	m_inputLayout;

		Microsoft::WRL::ComPtr <ID3D11Buffer> m_vsCbuffer;
		Microsoft::WRL::ComPtr <ID3D11Buffer> m_psCbuffer;
	};

	class SurfaceShader : public Shader {
	public:
		SurfaceShader(std::shared_ptr <Texture> _albedo, DirectX::XMFLOAT4 _color = DirectX::XMFLOAT4 { 1.0f, 1.0f, 1.0f, 1.0f });
		HRESULT LoadFromFile(LPCWSTR _fName, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext) override;

		float m_roughness;
		float m_metallic;
	
	protected:
		void SetBuffers(ID3D11DeviceContext* _pContext, DX::Camera& camera, DirectX::XMMATRIX _modelMat, uint32_t flags = SHADER_FLAGS_NONE) override;
	
	private:
		std::shared_ptr <Texture> m_albedo;
	};

	class FlatShader : public Shader {
	public:
		FlatShader(DirectX::XMFLOAT4 _color = DirectX::XMFLOAT4 { 1.0f, 1.0f, 1.0f, 1.0f });
		HRESULT LoadFromFile(LPCWSTR _fName, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext) override;
	
	protected:
		void SetBuffers(ID3D11DeviceContext* _pContext, DX::Camera& camera, DirectX::XMMATRIX _modelMat, uint32_t flags = SHADER_FLAGS_NONE) override;
	};
}