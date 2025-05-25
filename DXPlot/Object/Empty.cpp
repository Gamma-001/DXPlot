#pragma warning (disable: 26451)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Object/Empty.hpp>
#include <util.hpp>

#include <cmath>
#include <limits>

using namespace Cass;

// ---------- class Empty

Empty::Empty(size_t _vertCount) {
	m_vertCount = _vertCount;
}

Empty::~Empty() { }

void Empty::CreateBuffers() {
	m_vertCount = m_vertCount;
	m_vertexData = std::unique_ptr <detail::EMPTY_VERTEX_DATA[]>(new detail::EMPTY_VERTEX_DATA[m_vertCount]);

	D3D11_BUFFER_DESC bdc;
	ZeroMemory(&bdc, sizeof(bdc));
	bdc.Usage = D3D11_USAGE_DYNAMIC;
	bdc.ByteWidth = sizeof(detail::EMPTY_VERTEX_DATA) * m_vertCount;
	bdc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bdc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Cass::ThrowIfFailed(m_device->CreateBuffer(&bdc, nullptr, m_vertexBuffer.ReleaseAndGetAddressOf()));
}

void Empty::Render(Cass::Camera& camera, Cass::Shader& shader) {
	UINT strides = sizeof(detail::EMPTY_VERTEX_DATA);
	UINT offsets = 0;

	shader.SetActive(m_deviceContext.Get(), camera, m_transformation);

	m_deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &strides, &offsets);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	m_deviceContext->Draw(m_vertCount, 0);
}

void Empty::SetBuffers() {
	D3D11_MAPPED_SUBRESOURCE ms;

	// update buffers
	m_deviceContext->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, m_vertexData.get(), sizeof(detail::EMPTY_VERTEX_DATA) * m_vertCount);
	m_deviceContext->Unmap(m_vertexBuffer.Get(), 0);

	// update bounding box
	static float fMax = std::numeric_limits <float>::max();
	static float fMin = std::numeric_limits <float>::min();

	DirectX::XMFLOAT3 lb = { fMax , fMax, fMax }, ub = { fMin, fMin, fMin };
	for (int i = 0; i < m_vertCount; i++) {
		lb.x = std::min(lb.x, m_vertexData[i].position.x);
		lb.y = std::min(lb.y, m_vertexData[i].position.y);
		lb.z = std::min(lb.z, m_vertexData[i].position.z);

		ub.x = std::max(ub.x, m_vertexData[i].position.x);
		ub.y = std::max(ub.y, m_vertexData[i].position.y);
		ub.z = std::max(ub.z, m_vertexData[i].position.z);
	}

	m_bounds.Calculate(lb, ub);
}

// ---------- class Line

Line::Line(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b, DirectX::XMFLOAT4 _color, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext) : Empty(2) {
	m_points = std::make_pair (a, b);
	m_color = _color;


	CreateBuffers();
	InitVertices();
}

void Cass::Line::InitVertices() {
	m_vertexData[0].position = m_points.first;
	m_vertexData[1].position = m_points.second;

	m_vertexData[0].color = m_color;
	m_vertexData[1].color = m_color;

	SetBuffers();
}

// ---------- class Grid

Grid::Grid(float _width, float _height, uint32_t _resX, uint32_t _resY, uint32_t _skipOffset, DirectX::XMFLOAT4 _color, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext):
	Empty(((size_t)_resX + _resY) * 2U) {
	
	m_dims = { _width, _height };
	m_res = { _resX, _resY };
	m_skipOffset = std::max(1u, _skipOffset);
	m_color = _color;

	m_device = _pDevice;
	m_deviceContext = _pContext;

	CreateBuffers();
	InitVertices();
}

void Grid::InitVertices() {
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

	SetBuffers();
}

// ---------- class Box

Box::Box(DirectX::XMFLOAT3 _dims, DirectX::XMFLOAT4 _color, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext):Empty(24) {
	m_dims = _dims;
	m_color = _color;

	m_device = _pDevice;
	m_deviceContext = _pContext;

	CreateBuffers();
	InitVertices();
}

void Box::Recompute(DirectX::XMFLOAT3 _dims) {
	m_dims = _dims;
	InitVertices();
}

void Box::InitVertices() {
	int index = 0;
	for (int x = -1; x <= 1; x += 2) {
		for (int y = -1; y <= 1; y += 2) {
			m_vertexData[index++].position = { 0.5f * m_dims.x * x, 0.5f * m_dims.y * y, -0.5f * m_dims.z };
			m_vertexData[index++].position = { 0.5f * m_dims.x * x, 0.5f * m_dims.y * y, 0.5f * m_dims.z };
		}
	}
	for (int x = -1; x <= 1; x += 2) {
		for (int z = -1; z <= 1; z += 2) {
			m_vertexData[index++].position = { 0.5f * m_dims.x * x, -0.5f * m_dims.y, 0.5f * m_dims.z * z};
			m_vertexData[index++].position = { 0.5f * m_dims.x * x, 0.5f * m_dims.y, 0.5f * m_dims.z * z};
		}
	}
	for (int y = -1; y <= 1; y += 2) {
		for (int z = -1; z <= 1; z += 2) {
			m_vertexData[index++].position = { -0.5f * m_dims.x, 0.5f * m_dims.y * y, 0.5f * m_dims.z * z };
			m_vertexData[index++].position = { 0.5f * m_dims.x, 0.5f * m_dims.y * y, 0.5f * m_dims.z * z };
		}
	}

	for (int i = 0; i < 24; i++) {
		m_vertexData[i].color = m_color;
	}

	SetBuffers();
}