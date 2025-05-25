#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace Cass {
	// compute shader with one input and one output
	class ComputeShader {
	private:
		LPCWSTR fileName;
		LPCSTR entryPoint;

		Microsoft::WRL::ComPtr <ID3D11ComputeShader> m_computeShader;
		Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> m_SRV;
		Microsoft::WRL::ComPtr <ID3D11UnorderedAccessView> m_UAV;

	public:
		ComputeShader(LPCWSTR _fileName, LPCSTR _entryPoint, ID3D11Device*& _pDevice);

		// Create a read only Shader Resource view from the supplied resource
		void CreateSRVFromTexture(DXGI_FORMAT format, D3D11_SRV_DIMENSION dimension, ID3D11Device*& _pDevice, ID3D11Texture2D*& _pResource);

		// Create a read write Unordered Access view from the supplied resource
		void CreateUAVFromTexture(DXGI_FORMAT format, D3D11_UAV_DIMENSION dimension, ID3D11Device*& _pDevice, ID3D11Texture2D*& _pResource);

		// Dispatch the threads for execution
		void Execute(UINT groupCountX, UINT groupCountY, ID3D11DeviceContext*& _pContext);
	};
}