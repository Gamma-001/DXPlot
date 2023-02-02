#pragma warning (disable: 26451)

#ifndef NOMINMAX
#define NOMINMAX
#endif


#include "Object/Empty.hpp"
#include "util.hpp"

#include <cmath>

// ---------- class Empty

DX::Empty::Empty(size_t _vertCount, ID3D11Device* _pDevice) {
	m_vertCount = _vertCount;
	m_vertexData = std::unique_ptr <detail::EMPTY_VERTEX_DATA[]> (new detail::EMPTY_VERTEX_DATA[m_vertCount]);

	D3D11_BUFFER_DESC bdc;
	ZeroMemory(&bdc, sizeof(bdc));
	bdc.Usage = D3D11_USAGE_DYNAMIC;
	bdc.ByteWidth = sizeof(detail::EMPTY_VERTEX_DATA) * m_vertCount;
	bdc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bdc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	DX::ThrowIfFailed(_pDevice->CreateBuffer(&bdc, nullptr, m_vertexBuffer.ReleaseAndGetAddressOf()));
}

DX::Empty::~Empty() { }

void DX::Empty::Render(DX::Camera& camera, ID3D11DeviceContext* _pContext, DX::Shader& shader) {
	UINT strides = sizeof(detail::EMPTY_VERTEX_DATA);
	UINT offsets = 0;

	shader.SetActive(_pContext, camera, m_transformation);

	_pContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &strides, &offsets);
	_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	_pContext->Draw(m_vertCount, 0);
}

void DX::Empty::SetBuffers(ID3D11DeviceContext* _pContext) {
	D3D11_MAPPED_SUBRESOURCE ms;

	_pContext->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, m_vertexData.get(), sizeof(detail::EMPTY_VERTEX_DATA) * m_vertCount);
	_pContext->Unmap(m_vertexBuffer.Get(), 0);
}

// ---------- class Line

DX::Line::Line(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b, DirectX::XMFLOAT4 _color, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext) : Empty(2, _pDevice) {
	m_points = std::make_pair (a, b);
	m_color = _color;

	InitVertices(_pContext);
}

void DX::Line::InitVertices(ID3D11DeviceContext* _pContext) {
	m_vertexData[0].position = m_points.first;
	m_vertexData[1].position = m_points.second;

	m_vertexData[0].color = m_color;
	m_vertexData[1].color = m_color;

	SetBuffers(_pContext);
}

// ---------- class Grid

DX::Grid::Grid(float _width, float _height, uint32_t _resX, uint32_t _resY, uint32_t _skipOffset, DirectX::XMFLOAT4 _color, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext):
	Empty(((size_t)_resX + _resY) * 2U , _pDevice) {
	
	m_dims = { _width, _height };
	m_res = { _resX, _resY };
	m_skipOffset = std::max(1u, _skipOffset);
	m_color = _color;

	InitVertices(_pContext);
}

void DX::Grid::InitVertices(ID3D11DeviceContext* _pContext) {
	float offsetX = m_dims.x / (1 + m_res.x);
	float offsetY = m_dims.y / (1 + m_res.y);

	int index = 0;
	float x = -m_dims.x / 2;
	for (int i = 0; i < m_res.x; i++) {
		x += offsetX;

		m_vertexData[index++] = {
			{ x, -m_dims.y / 2, 0.0f },
			m_color
		};
		m_vertexData[index++] = {
			{ x, m_dims.y / 2, 0.0f },
			m_color
		};

		if ((i + m_res.x % 2) % m_skipOffset == 0) {
			m_vertexData[index - 1].color.w = 0.0f;
			m_vertexData[index - 2].color.w = 0.0f;
		}
	}

	float y = -m_dims.y / 2;
	for (int i = 0; i < m_res.y; i++) {
		y += offsetY;

		m_vertexData[index++] = {
			{ -m_dims.x / 2, y, 0.0f },
			m_color
		};
		m_vertexData[index++] = {
			{ m_dims.x / 2, y, 0.0f },
			m_color
		};

		if ((i + m_res.x % 2) % m_skipOffset == 0) {
			m_vertexData[index - 1].color.w = 0.0f;
			m_vertexData[index - 2].color.w = 0.0f;
		}
	}

	SetBuffers(_pContext);
}