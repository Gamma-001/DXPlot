#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Elements/BoundingBox.hpp>

#include <cmath>
#include <algorithm>

using namespace Cass;

BoundingBox::BoundingBox(DirectX::XMFLOAT3 _pos, DirectX::XMFLOAT3 _dims) {
	m_pos = _pos;
	m_dims = _dims;
}

void BoundingBox::Update(DirectX::XMFLOAT3 _pos, DirectX::XMFLOAT3 _dims) {
	m_pos = _pos;
	m_dims = _dims;
}

void BoundingBox::Calculate(DirectX::XMFLOAT3 _lb, DirectX::XMFLOAT3 _ub) {
	m_dims.x = std::max(abs(_ub.x - _lb.x), 0.1f);
	m_dims.y = std::max(abs(_ub.y - _lb.y), 0.1f);
	m_dims.z = std::max(abs(_ub.z - _lb.z), 0.1f);

	m_pos.x = 0.5f * (_ub.x + _lb.x);
	m_pos.y = 0.5f * (_ub.y + _lb.y);
	m_pos.z = 0.5f * (_ub.z + _lb.z);
}

int BoundingBox::Intersect(const Cass::Ray& _ray) const {
	if (m_dims.x <= 0.0f || m_dims.y <= 0.0f || m_dims.z <= 0.0f) return false;

	int count = 0;
	// bottom and upper quad
	for (int i = -1; i <= 1; i += 2) {
		count += _ray.IntersectTriangle(
			{ m_pos.x - m_dims.x * 0.5f, m_pos.y - m_dims.y * 0.5f, m_pos.z + i * m_dims.z * 0.5f },
			{ m_pos.x + m_dims.x * 0.5f, m_pos.y - m_dims.y * 0.5f, m_pos.z + i * m_dims.z * 0.5f },
			{ m_pos.x + m_dims.x * 0.5f, m_pos.y + m_dims.y * 0.5f, m_pos.z + i * m_dims.z * 0.5f }
		);
		count += _ray.IntersectTriangle(
			{ m_pos.x - m_dims.x * 0.5f, m_pos.y - m_dims.y * 0.5f, m_pos.z + i * m_dims.z * 0.5f },
			{ m_pos.x - m_dims.x * 0.5f, m_pos.y + m_dims.y * 0.5f, m_pos.z + i * m_dims.z * 0.5f },
			{ m_pos.x + m_dims.x * 0.5f, m_pos.y + m_dims.y * 0.5f, m_pos.z + i * m_dims.z * 0.5f }
		);
		if (count >= 2) return 1;
	}

	// front and back quad
	for (int i = -1; i <= 1; i += 2) {
		count += _ray.IntersectTriangle(
			{ m_pos.x - m_dims.x * 0.5f, m_pos.y + i * m_dims.y * 0.5f, m_pos.z - m_dims.z * 0.5f },
			{ m_pos.x + m_dims.x * 0.5f, m_pos.y + i * m_dims.y * 0.5f, m_pos.z - m_dims.z * 0.5f },
			{ m_pos.x + m_dims.x * 0.5f, m_pos.y + i * m_dims.y * 0.5f, m_pos.z + m_dims.z * 0.5f }
		);
		count += _ray.IntersectTriangle(
			{ m_pos.x - m_dims.x * 0.5f, m_pos.y + i * m_dims.y * 0.5f, m_pos.z - m_dims.z * 0.5f },
			{ m_pos.x - m_dims.x * 0.5f, m_pos.y + i * m_dims.y * 0.5f, m_pos.z + m_dims.z * 0.5f },
			{ m_pos.x + m_dims.x * 0.5f, m_pos.y + i * m_dims.y * 0.5f, m_pos.z + m_dims.z * 0.5f }
		);
		if (count >= 2) return 1;
	}

	// left and right quad
	for (int i = -1; i <= 1; i += 2) {
		count += _ray.IntersectTriangle(
			{ m_pos.x + i * m_dims.x * 0.5f, m_pos.y - m_dims.y * 0.5f, m_pos.z - m_dims.z * 0.5f },
			{ m_pos.x + i * m_dims.x * 0.5f, m_pos.y + m_dims.y * 0.5f, m_pos.z - m_dims.z * 0.5f },
			{ m_pos.x + i * m_dims.x * 0.5f, m_pos.y + m_dims.y * 0.5f, m_pos.z + m_dims.z * 0.5f }
		);
		count += _ray.IntersectTriangle(
			{ m_pos.x + i * m_dims.x * 0.5f, m_pos.y - m_dims.y * 0.5f, m_pos.z - m_dims.z * 0.5f },
			{ m_pos.x + i * m_dims.x * 0.5f, m_pos.y - m_dims.y * 0.5f, m_pos.z + m_dims.z * 0.5f },
			{ m_pos.x + i * m_dims.x * 0.5f, m_pos.y + m_dims.y * 0.5f, m_pos.z + m_dims.z * 0.5f }
		);
		if (count >= 2) return 1;
	}
	
	return -count;
}