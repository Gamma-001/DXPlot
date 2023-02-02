#pragma once

#include "Transform.hpp"
#include "Resource/Shader.hpp"
#include "Camera.hpp"

#include <d3d11.h>
#include <WRL/client.h>
#include <DirectXMath.h>

namespace DX {
	namespace detail {
		struct EMPTY_VERTEX_DATA {
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT4 color;
		};
	}

	class Empty : public Transform {
	public:
		Empty(size_t _vertCount, ID3D11Device* _pDevice);

		size_t GetVertCount() const { return m_vertCount; }
		void SetPosition(size_t _index, DirectX::XMFLOAT3 _pos) {
			if (_index >= 0 && _index < m_vertCount) m_vertexData[_index].position = _pos;
		}
		void SetColor(size_t _index, DirectX::XMFLOAT4 _color) {
			if (_index >= 0 && _index < m_vertCount) m_vertexData[_index].color = _color;
		}

		/**
		* @brief Update vertex buffer Data
		*/
		virtual void SetBuffers(ID3D11DeviceContext* _pContext);
		virtual void Render(Camera& camera, ID3D11DeviceContext* _pContext, Shader &shader);

		virtual ~Empty();

	protected:
		virtual void InitVertices(ID3D11DeviceContext* _pContext) = 0;

		size_t m_vertCount;
		std::unique_ptr <detail::EMPTY_VERTEX_DATA[]> m_vertexData;
		Microsoft::WRL::ComPtr <ID3D11Buffer> m_vertexBuffer;
	};

	class Line : public Empty {
	public:
		Line(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b, DirectX::XMFLOAT4 _color, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);

	protected:
		void InitVertices(ID3D11DeviceContext* _pContext) override;

	private:
		DirectX::XMFLOAT4 m_color;
		std::pair <DirectX::XMFLOAT3, DirectX::XMFLOAT3> m_points;
	};

	class Grid : public Empty {
	public:
		Grid(float _width, float _height, uint32_t _resX, uint32_t _resY, uint32_t _skipOffset, DirectX::XMFLOAT4 _color, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);

		DirectX::XMFLOAT4 GetColor() const { return m_color; }

	protected:
		void InitVertices(ID3D11DeviceContext* _pContext) override;

	private:
		DirectX::XMFLOAT4 m_color;
		DirectX::XMFLOAT2 m_dims;
		DirectX::XMUINT2 m_res;
		int m_skipOffset;
	};
}