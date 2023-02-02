#pragma once

#ifndef NOMIXMAX
#define NOMINMAX
#endif

#include <d3d11.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <DirectXMath.h>

#include <vector>

namespace DX {
	class Texture {
	public:
		Texture(D3D11_FILTER _filtering, D3D11_TEXTURE_ADDRESS_MODE _addressMode, DirectX::XMFLOAT4 _borderColor = { 0.0f, 0.0f, 0.0f, 0.0f }, size_t _width = 0, size_t _height = 0);

		inline ID3D11Texture2D* GetTexture2D() const { return m_texture.Get(); }
		inline ID3D11ShaderResourceView* GetSRV() const { return m_SRV.Get(); }
		inline ID3D11SamplerState* GetSampler() const { return m_sampler.Get(); }

		/**
		* @brief Create a mono color texture of specified dimensions
		*/
		HRESULT CreateFromSolidColor(
			uint32_t _width, uint32_t _height,
			uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a,
			ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext
		);
		/**
		* @brief Create a texture from specified file with generated mips if supported
		*/
		HRESULT LoadFromFile(LPCWSTR _fileName, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
		/**
		* @brief Create a texture from supplied color data, each group of 4 elements must contain RGBA values respectively
		*/
		HRESULT LoadFromMemory(
			uint32_t _width, uint32_t _height, std::vector<uint8_t> colorData, 
			ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext
		);

		static DXGI_FORMAT WICToDXGI(const GUID& guid);
		SIZE_T WICBitsPerPixel(const GUID& targetGuid);

	protected:
		HRESULT CreateTextureFromWIC(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, IWICBitmapFrameDecode* _pFrame);

		static Microsoft::WRL::ComPtr <IWICImagingFactory> s_factory;

		size_t m_width, m_height;
		D3D11_FILTER m_filtering;
		D3D11_TEXTURE_ADDRESS_MODE m_addressMode;
		DirectX::XMFLOAT4 m_borderColor;

		Microsoft::WRL::ComPtr <ID3D11Texture2D> m_texture;
		Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> m_SRV;
		Microsoft::WRL::ComPtr <ID3D11SamplerState> m_sampler;
	};
}