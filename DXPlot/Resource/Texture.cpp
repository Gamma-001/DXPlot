#include "Resource/Texture.hpp"
#include "util.hpp"

#include <combaseapi.h>
#include <wincodec.h>

#include <numeric>

namespace {
	struct WICTranslate {
		GUID                wic;
		DXGI_FORMAT         format;
	};

	struct WICConvert {
		GUID        source;
		GUID        target;
	};

	WICTranslate WICFormats[] = {
		{ GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },

		{ GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
		{ GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },

		{ GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
		{ GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM }, // DXGI 1.1
		{ GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM }, // DXGI 1.1

		{ GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM }, // DXGI 1.1
		{ GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },
		{ GUID_WICPixelFormat32bppRGBE,             DXGI_FORMAT_R9G9B9E5_SHAREDEXP },

#ifdef DXGI_1_2_FORMATS

		{ GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
		{ GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },

#endif

		{ GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
		{ GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
		{ GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
		{ GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },

		{ GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM },

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
		{ GUID_WICPixelFormat96bppRGBFloat,         DXGI_FORMAT_R32G32B32_FLOAT },
#endif
	};

	static WICConvert WICConversions[] = {
		{ GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

		{ GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
		{ GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
		{ GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
		{ GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 

		{ GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM 
		{ GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM 

		{ GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf }, // DXGI_FORMAT_R16_FLOAT 
		{ GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT 

#ifdef DXGI_1_2_FORMATS

		{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 }, // DXGI_FORMAT_B5G5R5A1_UNORM

#else

		{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat16bppBGRA5551,         GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat16bppBGR565,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

#endif

		{ GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 }, // DXGI_FORMAT_R10G10B10A2_UNORM

		{ GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
		{ GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
		{ GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
		{ GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 

		{ GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

		{ GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
		{ GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
		{ GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
		{ GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
		{ GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
		{ GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
		{ GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 

		{ GUID_WICPixelFormat96bppRGBFixedPoint,    GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
		{ GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
		{ GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
		{ GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
		{ GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 

		{ GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
		{ GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

	#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
		{ GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	#endif
	};
}

//
// ---------- class Texture
//

Microsoft::WRL::ComPtr <IWICImagingFactory> DX::Texture::s_factory = NULL;

DX::Texture::Texture(D3D11_FILTER _filtering, D3D11_TEXTURE_ADDRESS_MODE _addressMode, DirectX::XMFLOAT4 _borderColor, size_t _width, size_t _height) {
	m_width = _width;
	m_height = _height;
	m_filtering = _filtering;
	m_addressMode = _addressMode;
	m_borderColor = _borderColor;
}

DXGI_FORMAT DX::Texture::WICToDXGI(const GUID& guid) {
	for (SIZE_T i = 0; i < _countof(WICFormats); i++) {
		if (memcmp(&WICFormats[i], &guid, sizeof(GUID)) == 0) {
			return WICFormats[i].format;
		}
	}

	return DXGI_FORMAT_UNKNOWN;
}

SIZE_T DX::Texture::WICBitsPerPixel(const GUID& targetGuid) {
	if (!s_factory) return 0;

	Microsoft::WRL::ComPtr <IWICComponentInfo> cInfo;
	if (FAILED(s_factory->CreateComponentInfo(targetGuid, cInfo.ReleaseAndGetAddressOf()))) return 0;

	// reject invalid component type
	WICComponentType type;
	if (FAILED(cInfo->GetComponentType(&type))) return 0;
	if (type != WICPixelFormat) return 0;

	Microsoft::WRL::ComPtr <IWICPixelFormatInfo> pInfo;
	if (FAILED(cInfo->QueryInterface(__uuidof(IWICPixelFormatInfo), reinterpret_cast <void**> (pInfo.ReleaseAndGetAddressOf())))) return 0;

	UINT bpp = 0;
	if (FAILED(pInfo->GetBitsPerPixel(&bpp))) return 0;

	return bpp;
}

HRESULT DX::Texture::CreateFromSolidColor(
	uint32_t _width, uint32_t _height,
	uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a,
	ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext
) {
	if (_width == 0 || _height == 0) return E_INVALIDARG;

	std::vector <uint8_t> colorData(static_cast <size_t> (_width) * _height * 4, 0);
	for (size_t i = 0; i < colorData.size(); i += 4) {
		colorData[i] = _r;
		colorData[i + 1] = _g;
		colorData[i + 2] = _b;
		colorData[i + 3] = _a;
	}

	return LoadFromMemory(_width, _height, colorData, _pDevice, _pContext);
}

HRESULT DX::Texture::LoadFromFile(LPCWSTR _fileName, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext) {
	HRESULT hr = S_OK;

	// create the WIC factory if not initialized or invalid
	if (s_factory.Get() == nullptr) {
		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			__uuidof(IWICImagingFactory),
			reinterpret_cast <LPVOID*> (s_factory.ReleaseAndGetAddressOf())
		);
		if (FAILED(hr)) {
			s_factory.Reset();
			return hr;
		}
	}

	// create a bitmap decoder to retrieve the frame data
	Microsoft::WRL::ComPtr <IWICBitmapDecoder> decoder;
	hr = s_factory->CreateDecoderFromFilename(_fileName, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return hr;

	Microsoft::WRL::ComPtr <IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame(0, frame.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return hr;

	hr = CreateTextureFromWIC(_pDevice, _pContext, frame.Get());

	return hr;
}

HRESULT DX::Texture::LoadFromMemory(
	uint32_t _width, uint32_t _height, std::vector<uint8_t> colorData, 
	ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext
) {
	if (!_pDevice || !_pContext || colorData.size() < 4) return E_INVALIDARG;
#ifdef _M_ADM64
	if (colorData.size() > std::numeric_limits <uint16_t> ::max()) return HRESULT_FROM_WIN32( ERROR_FILE_TOO_LARGE );
#endif
	HRESULT hr = S_OK;
	size_t rowPitch = (_width * 32 + 7) / 8;
	size_t imageSize = rowPitch * _height;

	// Create the texture, SRV and sampler
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.Width = _width;
	td.Height = _height;
	td.MipLevels = 0;
	td.ArraySize = 1;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	td.CPUAccessFlags = NULL;
	td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	hr = _pDevice->CreateTexture2D(&td, nullptr, m_texture.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return hr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	ZeroMemory(&srvd, sizeof(srvd));
	srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MipLevels = -1;

	hr = _pDevice->CreateShaderResourceView(m_texture.Get(), &srvd, m_SRV.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		m_texture->Release();
		return hr;
	}
	
	// Generate Mips
	assert(_pContext != nullptr);
	_pContext->UpdateSubresource(m_texture.Get(), 0, nullptr, colorData.data(), static_cast <UINT> (rowPitch), static_cast <UINT> (imageSize));
	_pContext->GenerateMips(m_SRV.Get());

	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.Filter = m_filtering;
	sd.MaxAnisotropy = 0;
	sd.AddressU = m_addressMode;
	sd.AddressV = m_addressMode;
	sd.AddressW = m_addressMode;

	sd.BorderColor[0] = m_borderColor.x;
	sd.BorderColor[1] = m_borderColor.y;
	sd.BorderColor[2] = m_borderColor.z;
	sd.BorderColor[3] = m_borderColor.w;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;

	sd.MipLODBias = 0.0f;
	sd.MinLOD = 0;
	sd.MaxLOD = D3D11_FLOAT32_MAX;

	hr = _pDevice->CreateSamplerState(&sd, m_sampler.ReleaseAndGetAddressOf());

	m_width = _width;
	m_height = _height;

	return hr;
}

// Protected methods

HRESULT DX::Texture::CreateTextureFromWIC(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, IWICBitmapFrameDecode* _pFrame) {
	if (!_pContext || !_pFrame || !_pDevice) return E_INVALIDARG;

	UINT width, height;
	HRESULT hr = S_OK;

	hr = _pFrame->GetSize(&width, &height);
	if (FAILED(hr)) return hr;
	if (width == 0 || height == 0) return E_INVALIDARG;
	if (s_factory.Get() == nullptr) return E_NOINTERFACE;

	// Determine maximum supported texture size
	SIZE_T maxSize;
	switch (_pDevice->GetFeatureLevel()) {
	case D3D_FEATURE_LEVEL_9_1:
	case D3D_FEATURE_LEVEL_9_2:
		maxSize = 2048;
		break;

	case D3D_FEATURE_LEVEL_9_3:
		maxSize = 4096;
		break;

	case D3D_FEATURE_LEVEL_10_0:
	case D3D_FEATURE_LEVEL_10_1:
		maxSize = 8192;
		break;

	default:
		maxSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
		break;
	}
	assert(maxSize > 0);

	// DownScale the target size if too large
	UINT tWidth, tHeight;
	if (width > maxSize || height > maxSize) {
		float aspect = static_cast <float> (width) / static_cast <float> (height);
		if (height > width) {
			tHeight = static_cast <UINT> (maxSize);
			tWidth = static_cast <UINT> (static_cast <float> (tHeight) * aspect);
		}
		else {
			tWidth = static_cast <UINT> (maxSize);
			tHeight = static_cast <UINT> (static_cast <float> (tWidth) / aspect);
		}
		assert(tWidth <= maxSize && tHeight <= maxSize);
	}
	else {
		tWidth = width;
		tHeight = height;
	}

	// Determine pixel format
	WICPixelFormatGUID pixelFormat;
	hr = _pFrame->GetPixelFormat(&pixelFormat);
	if (FAILED(hr)) return hr;

	WICPixelFormatGUID convertGuid;
	memcpy(&convertGuid, &pixelFormat, sizeof(WICPixelFormatGUID));

	SIZE_T bpp;
	DXGI_FORMAT format = WICToDXGI(pixelFormat);

	// Test if format conversion is possible
	if (format == DXGI_FORMAT_UNKNOWN) {
		for (size_t i = 0; i < _countof(WICConversions); i++) {
			if (memcmp(&pixelFormat, &WICConversions[i].source, sizeof(WICPixelFormatGUID)) == 0) {
				memcpy(&convertGuid, &WICConversions[i].target, sizeof(WICPixelFormatGUID));
				format = WICToDXGI(convertGuid);
				bpp = WICBitsPerPixel(convertGuid);

				break;
			}
		}

		if (format == DXGI_FORMAT_UNKNOWN) return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	}
	else {
		bpp = WICBitsPerPixel(pixelFormat);
	}
	if (!bpp) return E_FAIL;

	// Verify the target format is supported by the current device
	UINT support = 0;
	hr = _pDevice->CheckFormatSupport(format, &support);
	if (FAILED(hr) || !(D3D11_FORMAT_SUPPORT_TEXTURE2D & support)) {
		// Fallback to RGBA 32-bit format which is supported by all devices
		memcpy(&convertGuid, &GUID_WICPixelFormat32bppRGBA, sizeof(WICPixelFormatGUID));
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bpp = 32;
	}

	// Allocate temporary memory for image
	SIZE_T rowPitch = (tWidth * bpp + 7) / 8;
	SIZE_T imageSize = rowPitch * tHeight;
	std::unique_ptr <uint8_t[]> temp(new uint8_t[imageSize]);

	// No format conversion or resize
	if (memcmp(&pixelFormat, &convertGuid, sizeof(WICPixelFormatGUID)) == 0 && tWidth == width && tHeight == height) {
		hr = _pFrame->CopyPixels(NULL, static_cast <UINT> (rowPitch), static_cast <UINT> (imageSize), temp.get());
		if (FAILED(hr)) return hr;
	}
	// Apply resize
	else if (tWidth != width || tHeight != height) {
		Microsoft::WRL::ComPtr <IWICBitmapScaler> scaler;
		hr = s_factory->CreateBitmapScaler(scaler.ReleaseAndGetAddressOf());
		if (FAILED(hr)) return hr;

		hr = scaler->Initialize(_pFrame, tWidth, tHeight, WICBitmapInterpolationModeFant);
		if (FAILED(hr)) return hr;

		WICPixelFormatGUID pfScaler;
		hr = scaler->GetPixelFormat(&pfScaler);
		if (FAILED(hr)) return hr;

		// No format conversion needed
		if (memcmp(&convertGuid, &pfScaler, sizeof(WICPixelFormatGUID)) == 0) {
			hr = scaler->CopyPixels(NULL, static_cast <UINT> (rowPitch), static_cast <UINT> (imageSize), temp.get());
			if (FAILED(hr)) return hr;
		}
		// Format conversion on resized texture
		else {
			Microsoft::WRL::ComPtr <IWICFormatConverter> converter;
			hr = s_factory->CreateFormatConverter(converter.ReleaseAndGetAddressOf());
			if (FAILED(hr)) return hr;

			hr = converter->Initialize(_pFrame, convertGuid, WICBitmapDitherTypeErrorDiffusion, NULL, NULL, WICBitmapPaletteTypeCustom);
			if (FAILED(hr)) return hr;

			hr = converter->CopyPixels(NULL, static_cast <UINT> (rowPitch), static_cast <UINT> (imageSize), temp.get());
			if (FAILED(hr)) return hr;
		}
	}
	// Format conversion without resize
	else {
		Microsoft::WRL::ComPtr <IWICFormatConverter> converter;
		hr = s_factory->CreateFormatConverter(converter.ReleaseAndGetAddressOf());
		if (FAILED(hr)) return hr;

		hr = converter->Initialize(_pFrame, convertGuid, WICBitmapDitherTypeErrorDiffusion, NULL, NULL, WICBitmapPaletteTypeCustom);
		if (FAILED(hr)) return hr;

		hr = converter->CopyPixels(NULL, static_cast <UINT> (rowPitch), static_cast <UINT> (imageSize), temp.get());
		if (FAILED(hr)) return hr;
	}

	// Test for mip map auto-gen support
	bool autogen = false;
	UINT fmSupport = 0;
	hr = _pDevice->CheckFormatSupport(format, &fmSupport);
	if (SUCCEEDED(hr) && (fmSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN)) {
		autogen = true;
	}

	// Create the texture, SRV and sampler
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Format = format;
	td.Width = tWidth;
	td.Height = tHeight;
	td.MipLevels = autogen ? 0 : 1;
	td.ArraySize = 1;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = autogen ? (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET) : D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = NULL;
	td.MiscFlags = autogen ? D3D11_RESOURCE_MISC_GENERATE_MIPS : NULL;

	D3D11_SUBRESOURCE_DATA texInit;
	ZeroMemory(&texInit, sizeof(texInit));
	texInit.pSysMem = temp.get();
	texInit.SysMemPitch = static_cast <UINT> (rowPitch);
	texInit.SysMemSlicePitch = static_cast <UINT> (imageSize);

	hr = _pDevice->CreateTexture2D(&td, autogen ? nullptr : &texInit, m_texture.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return hr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	ZeroMemory(&srvd, sizeof(srvd));
	srvd.Format = format;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MipLevels = autogen ? -1 : 1;

	hr = _pDevice->CreateShaderResourceView(m_texture.Get(), &srvd, m_SRV.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		m_texture->Release();
		return hr;
	}

	// Generate Mips
	if (autogen) {
		assert(_pContext != nullptr);
		_pContext->UpdateSubresource(m_texture.Get(), 0, nullptr, temp.get(), static_cast <UINT> (rowPitch), static_cast <UINT> (imageSize));
		_pContext->GenerateMips(m_SRV.Get());
	}

	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.Filter = m_filtering;
	sd.MaxAnisotropy = 0;
	sd.AddressU = m_addressMode;
	sd.AddressV = m_addressMode;
	sd.AddressW = m_addressMode;

	sd.BorderColor[0] = m_borderColor.x;
	sd.BorderColor[1] = m_borderColor.y;
	sd.BorderColor[2] = m_borderColor.z;
	sd.BorderColor[3] = m_borderColor.w;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;

	sd.MipLODBias = 0.0f;
	sd.MinLOD = 0;
	sd.MaxLOD = D3D11_FLOAT32_MAX;

	hr = _pDevice->CreateSamplerState(&sd, m_sampler.ReleaseAndGetAddressOf());

	m_width = tWidth;
	m_height = tHeight;

	return hr;
}