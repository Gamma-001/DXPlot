#include <Transform.hpp>
#include <util.hpp>

#include <DirectXMath.h>

using namespace Cass;

Transform::Transform() {
	m_transformation = DirectX::XMMatrixIdentity();
}

Transform::~Transform() { }

DirectX::XMFLOAT3 Transform::GetPosition() const {
	DirectX::XMVECTOR pos, t_rot, t_scale;
	DirectX::XMFLOAT3 res;

	DirectX::XMMatrixDecompose(&t_scale, &t_rot, &pos, m_transformation);
	DirectX::XMStoreFloat3(&res, pos);

	return res;
}

DirectX::XMFLOAT4 Transform::GetRotationQuat() const {
	DirectX::XMVECTOR t_pos, rot, t_scale;
	DirectX::XMFLOAT4 res;

	DirectX::XMMatrixDecompose(&t_scale, &rot, &t_pos, m_transformation);
	DirectX::XMStoreFloat4(&res, rot);

	return res;
}

DirectX::XMFLOAT3 Transform::GetRotationEuler() const {
	DirectX::XMVECTOR t_pos, rot, t_scale, res_vec;
	DirectX::XMFLOAT3 res;
	float rot_angle;

	DirectX::XMMatrixDecompose(&t_scale, &rot, &t_pos, m_transformation);
	DirectX::XMQuaternionToAxisAngle(&res_vec, &rot_angle, rot);
	DirectX::XMStoreFloat3(&res, res_vec);

	return DirectX::XMFLOAT3{ res.x * rot_angle * Math::_180_P, res.y * rot_angle * Math::_180_P, res.z * rot_angle * Math::_180_P };
}

DirectX::XMFLOAT3 Transform::GetScale() const {
	DirectX::XMVECTOR t_pos, t_rot, scale;
	DirectX::XMFLOAT3 res;

	DirectX::XMMatrixDecompose(&scale, &t_rot, &t_pos, m_transformation);
	DirectX::XMStoreFloat3(&res, scale);

	return res;
}

void Transform::ResetTransform() {
	m_transformation = DirectX::XMMatrixIdentity();
}

void Transform::Translate(DirectX::XMFLOAT3 _offset) {
	m_transformation = DirectX::XMMatrixMultiply(
		m_transformation,
		DirectX::XMMatrixTranslation(_offset.x, _offset.y, _offset.z)
	);
}

void Transform::Rotate(DirectX::XMFLOAT3 _axis, float _angleEuler) {
	m_transformation = DirectX::XMMatrixMultiply(
		DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&_axis), _angleEuler * Math::PI_180),
		m_transformation
	);
}

void Transform::Scale(DirectX::XMFLOAT3 _axis) {
	m_transformation = DirectX::XMMatrixMultiply(
		DirectX::XMMatrixScaling(_axis.x, _axis.y, _axis.z),
		m_transformation
	);
}

int Transform::IntersectBox(const Ray& ray) const {
	return m_bounds.Intersect(ray);
}