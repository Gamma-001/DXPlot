#include "Resource/DeviceResources.hpp"
#include "util.hpp"

#include <sdkddkver.h>
#include <dxgi.h>

using namespace Cass;

Microsoft::WRL::ComPtr <IDXGIFactory1> DeviceResources::s_factory = nullptr;

// public methods

DeviceResources::DeviceResources(
	DXGI_FORMAT _backBufferFormat,
	DXGI_FORMAT _depthBufferFormat,
	UINT _backBufferCount,
	D3D_FEATURE_LEVEL _minFeatureLevel,
	bool _msaa
) :
	m_backBufferFormat(_backBufferFormat),
	m_depthBufferFormat(_depthBufferFormat),
	m_backBufferCount(_backBufferCount),
	m_minFeatureLevel(_minFeatureLevel),
	m_window(NULL),
	m_windowSize({ 0, 0, 1, 1 }),
	m_featureLevel(D3D_FEATURE_LEVEL_9_1),
	m_msaaEnabled(_msaa) {
	
	ZeroMemory(&m_viewport, sizeof(m_viewport));
}

DeviceResources::~DeviceResources() { }

void DeviceResources::CreateDeviceResource(UINT creationFlags) {
#if defined _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};
	
	// determine the number of feature levels greater than minimum specified
	UINT featureLevelCount = 0;
	for (; featureLevelCount < _countof(featureLevels); featureLevelCount++) {
		if (featureLevels[featureLevelCount] < m_minFeatureLevel)
			break;
	}
	if (featureLevelCount == 0) {
		throw std::out_of_range("minFeatureLevel too high");
	}
	
	HRESULT hr = S_OK;
	hr = D3D11CreateDevice(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		creationFlags,
		featureLevels,
		featureLevelCount,
		D3D11_SDK_VERSION,
		m_device.GetAddressOf(),
		&m_featureLevel,
		m_context.GetAddressOf()
	);
	ThrowIfFailed(hr);

	// retrieve the factory

	if (s_factory.Get() == nullptr) {
		Microsoft::WRL::ComPtr <IDXGIDevice1> dxgiDevice;
		m_device.As(&dxgiDevice);

		Microsoft::WRL::ComPtr <IDXGIAdapter> dxgiAdapter;
		dxgiDevice->GetAdapter(&dxgiAdapter);

		ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), &s_factory));
	}
	CreateStates();
}

void DeviceResources::CreateSizeDependentResource() {
	if (!m_window) {
		throw std::exception("window handle not set");
	}

	// clear the previous window size specific context
	ID3D11RenderTargetView* nullviews[] = { nullptr };
	m_context->Flush();
	m_context->OMSetRenderTargets(_countof(nullviews), nullviews, nullptr);
	m_RTV.Reset();
	m_DSV.Reset();
	m_renderTarget.Reset();
	m_depthStencil.Reset();

	UINT nWidth = std::max <UINT> (static_cast <UINT> (m_windowSize.right - m_windowSize.left), 100u);
	UINT nHeight = std::max <UINT> (static_cast <UINT> (m_windowSize.bottom - m_windowSize.top), 100u);

	// if the swap chain already exists, resize it 
	if (m_swapChain) {
		HRESULT hr = m_swapChain->ResizeBuffers(
			m_backBufferCount,
			nWidth,
			nHeight,
			m_backBufferFormat,
			NULL
		);
		ThrowIfFailed(hr);
	}

	// else create the swap chain
	else {
		DXGI_SWAP_CHAIN_DESC scd;
		ZeroMemory(&scd, sizeof(scd));

		scd.BufferCount = m_backBufferCount;
		scd.BufferDesc.Width = nWidth;
		scd.BufferDesc.Height = nHeight;
		scd.BufferDesc.Format = m_backBufferFormat;
		scd.BufferDesc.RefreshRate.Numerator = 1;
		scd.BufferDesc.RefreshRate.Denominator = 60;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

		scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		scd.SampleDesc.Count = 1;
		scd.SampleDesc.Quality = 0;
		scd.OutputWindow = m_window;
		scd.Windowed = TRUE;

		ThrowIfFailed(s_factory->CreateSwapChain(m_device.Get(), &scd, m_swapChain.ReleaseAndGetAddressOf()));
	}
	
	// disable ALT + Enter shortcut
	ThrowIfFailed(s_factory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));

	// create the render target view of the swap chain back buffer
	ThrowIfFailed(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast <void**> (m_renderTarget.ReleaseAndGetAddressOf())));

	CD3D11_RENDER_TARGET_VIEW_DESC rtvd(D3D11_RTV_DIMENSION_TEXTURE2D, m_backBufferFormat);
	ThrowIfFailed(m_device->CreateRenderTargetView(m_renderTarget.Get(), &rtvd, m_RTV.ReleaseAndGetAddressOf()));

	// create the depth stencil and its view
	if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN) {
		CD3D11_TEXTURE2D_DESC dsd(
			m_msaaEnabled ? DXGI_FORMAT_R24G8_TYPELESS : m_depthBufferFormat,
			nWidth,
			nHeight,
			1,
			1,
			D3D11_BIND_DEPTH_STENCIL
		);
		ThrowIfFailed(m_device->CreateTexture2D(&dsd, nullptr, m_depthStencil.ReleaseAndGetAddressOf()));

		CD3D11_DEPTH_STENCIL_VIEW_DESC dsvd(D3D11_DSV_DIMENSION_TEXTURE2D, m_depthBufferFormat);
		ThrowIfFailed(m_device->CreateDepthStencilView(m_depthStencil.Get(), &dsvd, m_DSV.ReleaseAndGetAddressOf()));
	}
	
	// set the viewport
	m_viewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		static_cast <float> (nWidth),
		static_cast <float> (nHeight)
	);

	if (m_msaaEnabled)
		CreateRenderTargets_msaa();
}

void DeviceResources::SetRenderTarget_no_msaa(bool _culling, bool _depthEnable) {
	if (_culling) m_context->RSSetState(m_RS_no_msaa.Get());
	else m_context->RSSetState(m_RS_no_msaa_no_cull.Get());

	m_context->OMSetDepthStencilState(m_DSS.Get(), 1);
	m_context->OMSetRenderTargets(1, m_RTV.GetAddressOf(), _depthEnable ? m_DSV.Get() : nullptr);
	m_context->OMSetBlendState(m_blendState.Get(), NULL, 0xffffffff);
}

void DeviceResources::SetRenderTarget_msaa(bool _culling, bool _depthEnable, bool _lineAA) {
	if (!m_msaaEnabled) return;

	if (_lineAA) m_context->RSSetState(m_RS_msaa_lineAA.Get());
	else if (_culling) m_context->RSSetState(m_RS_msaa.Get());
	else m_context->RSSetState(m_RS_msaa_no_cull.Get());

	m_context->OMSetDepthStencilState(m_DSS.Get(), 1);
	m_context->OMSetRenderTargets(1, m_RTV_msaa.GetAddressOf(), _depthEnable ? m_DSV_msaa.Get() : nullptr);
	m_context->OMSetBlendState(m_blendState.Get(), NULL, 0xffffffff);
}

void DeviceResources::SetViewport() {
	m_context->RSSetViewports(1, &m_viewport);
}

void DeviceResources::Clear(const float _clearColor[4]) {
	m_context->ClearRenderTargetView(m_RTV.Get(), _clearColor);
	m_context->ClearDepthStencilView(m_DSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0U);

	if (m_msaaEnabled) {
		m_context->ClearRenderTargetView(m_RTV_msaa.Get(), _clearColor);
		m_context->ClearDepthStencilView(m_DSV_msaa.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0U);
	}
}

void DeviceResources::Resolve() {
	m_context->ResolveSubresource(m_renderTarget.Get(), 0, m_renderTarget_msaa.Get(), 0, m_backBufferFormat);
}

void DeviceResources::SetWindow(HWND _hWnd, int _width, int _height) {
	m_window = _hWnd;
	m_windowSize.left = m_windowSize.top = 0;
	m_windowSize.right = _width;
	m_windowSize.bottom = _height;
}

void DeviceResources::WindowSizeChanged(int _width, int _height) {
	m_windowSize.left = m_windowSize.top = 0;
	m_windowSize.right = _width;
	m_windowSize.bottom = _height;

	CreateSizeDependentResource();
}

HRESULT DeviceResources::CreateTexture2D(_In_ UINT _width, _In_ UINT _height, _In_ DXGI_FORMAT _format, _In_ UINT _bindFlags, _Out_ ID3D11Texture2D*& _texture) {
	if (!_width || !_height)
		throw std::invalid_argument("height or width can't be 0");

	CD3D11_TEXTURE2D_DESC td(
		_format,
		_width,
		_height,
		1U,
		1U,
		_bindFlags
	);
	ThrowIfFailed(m_device->CreateTexture2D(&td, nullptr, &_texture));

	return S_OK;
}

//
// private methods
//

void DeviceResources::CreateStates() {
	if (!m_device) {
		throw std::exception("Device not set or destroyed");
	}

	// raster state

	D3D11_RASTERIZER_DESC rd;
	ZeroMemory(&rd, sizeof(rd));

	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_BACK;
	rd.FrontCounterClockwise = FALSE;
	rd.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
	rd.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rd.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
	rd.DepthClipEnable = TRUE;
	rd.ScissorEnable = FALSE;
	rd.MultisampleEnable = FALSE;
	rd.AntialiasedLineEnable = FALSE;

	D3D11_RASTERIZER_DESC rd1(rd);
	rd1.CullMode = D3D11_CULL_NONE;

	ThrowIfFailed(m_device->CreateRasterizerState(&rd, m_RS_no_msaa.ReleaseAndGetAddressOf()));
	ThrowIfFailed(m_device->CreateRasterizerState(&rd1, m_RS_no_msaa_no_cull.ReleaseAndGetAddressOf()));

	if (m_msaaEnabled) {
		D3D11_RASTERIZER_DESC rd2(rd);
		rd2.MultisampleEnable = TRUE;

		D3D11_RASTERIZER_DESC rd3(rd2);
		rd3.CullMode = D3D11_CULL_NONE;

		D3D11_RASTERIZER_DESC rd4(rd3);
		rd4.AntialiasedLineEnable = TRUE;

		ThrowIfFailed(m_device->CreateRasterizerState(&rd2, m_RS_msaa.ReleaseAndGetAddressOf()));
		ThrowIfFailed(m_device->CreateRasterizerState(&rd3, m_RS_msaa_no_cull.ReleaseAndGetAddressOf()));
		ThrowIfFailed(m_device->CreateRasterizerState(&rd4, m_RS_msaa_lineAA.ReleaseAndGetAddressOf()));
	}

	// depth stencil state
	D3D11_DEPTH_STENCIL_DESC dsd;
	ZeroMemory(&dsd, sizeof(dsd));

	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;

	dsd.StencilEnable = TRUE;
	dsd.StencilReadMask = 0xff;
	dsd.StencilWriteMask = 0xff;

	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

	ThrowIfFailed(m_device->CreateDepthStencilState(&dsd, m_DSS.ReleaseAndGetAddressOf()));

	D3D11_BLEND_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;
	bd.RenderTarget[0].BlendEnable = TRUE;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ThrowIfFailed(m_device->CreateBlendState(&bd, m_blendState.ReleaseAndGetAddressOf()));
}

void DeviceResources::CreateRenderTargets_msaa() {
	m_depthStencil_msaa.Reset();
	m_renderTarget_msaa.Reset();
	m_DSV_msaa.Reset();
	m_RTV_msaa.Reset();

	UINT nWidth = std::max <UINT>(static_cast <UINT> (m_windowSize.right - m_windowSize.left), 1u);
	UINT nHeight = std::max <UINT>(static_cast <UINT> (m_windowSize.bottom - m_windowSize.top), 1u);

	// render target descriptors
	CD3D11_TEXTURE2D_DESC rtd(
		m_backBufferFormat,
		nWidth,
		nHeight,
		1,
		1,
		D3D11_BIND_RENDER_TARGET,
		D3D11_USAGE_DEFAULT,
		0U,
		4U,
		0U
	);
	D3D11_RENDER_TARGET_VIEW_DESC rtvd;
	ZeroMemory(&rtvd, sizeof(rtvd));
	rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	rtvd.Format = m_backBufferFormat;
	rtvd.Texture2DMS = { 0 };

	// depth stecil descriptors
	D3D11_TEXTURE2D_DESC dsd = rtd;
	dsd.Format = DXGI_FORMAT_R24G8_TYPELESS;
	dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(dsvd));
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	dsvd.Format = m_depthBufferFormat;
	dsvd.Texture2DMS = { 0 };

	// create the resources
	ThrowIfFailed(m_device->CreateTexture2D(&rtd, nullptr, m_renderTarget_msaa.ReleaseAndGetAddressOf()));
	ThrowIfFailed(m_device->CreateRenderTargetView(m_renderTarget_msaa.Get(), &rtvd, m_RTV_msaa.ReleaseAndGetAddressOf()));

	ThrowIfFailed(m_device->CreateTexture2D(&dsd, nullptr, m_depthStencil_msaa.ReleaseAndGetAddressOf()));
	ThrowIfFailed(m_device->CreateDepthStencilView(m_depthStencil_msaa.Get(), &dsvd, m_DSV_msaa.ReleaseAndGetAddressOf()));
}