#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace Cass {
	class DeviceResources {
	public:
		DeviceResources(
			DXGI_FORMAT _backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT _depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
			UINT _backBufferCount = 1U,
			D3D_FEATURE_LEVEL _minFeatureLevel = D3D_FEATURE_LEVEL_11_0,
			bool _MSAA = true
		);
		~DeviceResources();

		ID3D11Device* GetDevice() const { return m_device.Get(); };
		IDXGISwapChain* GetSwapChain() const { return m_swapChain.Get(); }
		ID3D11DeviceContext* GetDeviceContext() const { return m_context.Get(); }
		ID3D11Texture2D* GetRenderTarget() const { return m_renderTarget.Get(); }
		ID3D11Texture2D* GetDepthStencil() const { return m_depthStencil.Get(); }
		ID3D11Texture2D* GetDepthStencil_msaa() const { return m_depthStencil_msaa.Get(); }
		ID3D11RenderTargetView* GetRTV() const { return m_RTV.Get(); }
		ID3D11DepthStencilView* GetDSV() const { return m_DSV.Get(); }

		RECT GetClientRect() const { return m_windowSize; }

		/** 
		* @brief Create size Independent device resources
		*/
		void CreateDeviceResource(UINT creationFlags);

		/**
		* @brief Called every time window size changes
		*/
		void CreateSizeDependentResource();

		void SetRenderTarget_no_msaa(bool _culling = true, bool _depthEnable = true);
		void SetRenderTarget_msaa(bool _culling = true, bool _depthEnable = true, bool _lineAA = false);
		void SetViewport();
		void Clear(const float _clearColor[4]);

		/**
		* @brief Resolve rendered data from multi sampled render target to non multisampled render target
		*/
		void Resolve();

		void SetWindow(HWND _hWnd, int _width, int _height);
		void WindowSizeChanged(int _width, int _height);

		HRESULT CreateTexture2D(_In_ UINT _width, _In_ UINT _height, _In_ DXGI_FORMAT _format, _In_ UINT _bindFlags, _Out_ ID3D11Texture2D*& _texture);

	private:
		void CreateStates();
		void CreateRenderTargets_msaa();

		static Microsoft::WRL::ComPtr <IDXGIFactory1>		s_factory;

		Microsoft::WRL::ComPtr <ID3D11Device>				m_device;
		Microsoft::WRL::ComPtr <IDXGISwapChain>				m_swapChain;
		Microsoft::WRL::ComPtr <ID3D11DeviceContext>		m_context;
		
		Microsoft::WRL::ComPtr <ID3D11Texture2D>			m_renderTarget;
		Microsoft::WRL::ComPtr <ID3D11Texture2D>			m_depthStencil;
		Microsoft::WRL::ComPtr <ID3D11RenderTargetView>		m_RTV;
		Microsoft::WRL::ComPtr <ID3D11DepthStencilView>		m_DSV;

		Microsoft::WRL::ComPtr <ID3D11Texture2D>			m_renderTarget_msaa;
		Microsoft::WRL::ComPtr <ID3D11Texture2D>			m_depthStencil_msaa;
		Microsoft::WRL::ComPtr <ID3D11RenderTargetView>		m_RTV_msaa;
		Microsoft::WRL::ComPtr <ID3D11DepthStencilView>		m_DSV_msaa;

		Microsoft::WRL::ComPtr <ID3D11RasterizerState>		m_RS_no_msaa;
		Microsoft::WRL::ComPtr <ID3D11RasterizerState>		m_RS_no_msaa_no_cull;
		Microsoft::WRL::ComPtr <ID3D11RasterizerState>		m_RS_msaa;
		Microsoft::WRL::ComPtr <ID3D11RasterizerState>		m_RS_msaa_no_cull;
		Microsoft::WRL::ComPtr <ID3D11RasterizerState>		m_RS_msaa_lineAA;
		Microsoft::WRL::ComPtr <ID3D11DepthStencilState>	m_DSS;
		Microsoft::WRL::ComPtr <ID3D11BlendState>			m_blendState;

		HWND					m_window;
		RECT					m_windowSize;
		UINT					m_backBufferCount;
		DXGI_FORMAT				m_backBufferFormat;
		DXGI_FORMAT				m_depthBufferFormat;
		D3D_FEATURE_LEVEL		m_minFeatureLevel;
		D3D_FEATURE_LEVEL		m_featureLevel;
		D3D11_VIEWPORT			m_viewport;

		bool m_msaaEnabled;
	};
}