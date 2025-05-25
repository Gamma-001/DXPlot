#include <Elements/Ray.hpp>
#include <util.hpp>

using namespace Cass;

Ray::Ray(DirectX::XMFLOAT3 _origin, DirectX::XMFLOAT3 _dir) {
	m_origin = _origin;
	m_dir = _dir;

	if (_dir.x == 0.0f && _dir.y == 0.0f && _dir.z == 0.0f) return;
	DirectX::XMVECTOR dir_vec = DirectX::XMLoadFloat3(&_dir);
	DirectX::XMStoreFloat3(&m_dir, DirectX::XMVector3Normalize(dir_vec));
}

void Ray::Update(DirectX::XMFLOAT3 _origin = { 0.0f, 0.0f, 0.0f }, DirectX::XMFLOAT3 _dir = { 0.0f, 0.0f, 0.0f }) {
	m_origin = _origin;
	m_dir = _dir;

	if (_dir.x == 0.0f && _dir.y == 0.0f && _dir.z == 0.0f) return;
	DirectX::XMVECTOR dir_vec = DirectX::XMLoadFloat3(&_dir);
	DirectX::XMStoreFloat3(&m_dir, DirectX::XMVector3Normalize(dir_vec));
}

bool Ray::IntersectTriangle(DirectX::XMFLOAT3 _a, DirectX::XMFLOAT3 _b, DirectX::XMFLOAT3 _c) const {
	if (m_dir.x == 0.0f && m_dir.y == 0.0f && m_dir.z == 0.0f) return false;

	// Moller-Trumbore ray-triangle intersection

	static float EPSILON = 0.0000001f;
	DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&_a), v1 = DirectX::XMLoadFloat3(&_b), v2 = DirectX::XMLoadFloat3(&_c);
	DirectX::XMVECTOR ray_vec = DirectX::XMLoadFloat3(&m_dir);
	DirectX::XMVECTOR o_vec = DirectX::XMLoadFloat3(&m_origin);

	DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(v1, v0);
	DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(v2, v0);
	DirectX::XMVECTOR h = DirectX::XMVector3Cross(ray_vec, edge2);
	float a = DirectX::XMVectorGetX(DirectX::XMVector3Dot(edge1, h));
	if (a > -EPSILON && a < EPSILON) return false;	// ray parallel to the triangle

	float f = 1.0f / a;
	DirectX::XMVECTOR s = DirectX::XMVectorSubtract(o_vec, v0);
	float u = f * DirectX::XMVectorGetX(DirectX::XMVector3Dot(s, h));
	if (u < 0.0f || u > 1.0f) return false;

	DirectX::XMVECTOR q = DirectX::XMVector3Cross(s, edge1);
	float v = f * DirectX::XMVectorGetX(DirectX::XMVector3Dot(ray_vec, q));
	if (v < 0.0f || u + v > 1.0f) return false;

	float t = f * DirectX::XMVectorGetX(DirectX::XMVector3Dot(edge2, q));
	if (t > EPSILON) return true;

	return false;
}