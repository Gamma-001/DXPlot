#include "Transform.hpp"
#include "util.hpp"

#include <DirectXMath.h>

DX::Transform::Transform() {
	m_transformation = DirectX::XMMatrixIdentity();
}

DX::Transform::~Transform() {

}

DirectX::XMFLOAT3 DX::Transform::GetPosition() const {
	DirectX::XMVECTOR pos, t_rot, t_scale;
	DirectX::XMFLOAT3 res;

	DirectX::XMMatrixDecompose(&t_scale, &t_rot, &pos, m_transformation);
	DirectX::XMStoreFloat3(&res, pos);

	return res;
}

DirectX::XMFLOAT4 DX::Transform::GetRotationQuat() const {
	DirectX::XMVECTOR t_pos, rot, t_scale;
	DirectX::XMFLOAT4 res;

	DirectX::XMMatrixDecompose(&t_scale, &rot, &t_pos, m_transformation);
	DirectX::XMStoreFloat4(&res, rot);

	return res;
}

DirectX::XMFLOAT3 DX::Transform::GetRotationEuler() const {
	DirectX::XMVECTOR t_pos, rot, t_scale, res_vec;
	DirectX::XMFLOAT3 res;
	float rot_angle;

	DirectX::XMMatrixDecompose(&t_scale, &rot, &t_pos, m_transformation);
	DirectX::XMQuaternionToAxisAngle(&res_vec, &rot_angle, rot);
	DirectX::XMStoreFloat3(&res, res_vec);

	return DirectX::XMFLOAT3{ res.x * rot_angle * DX::Math::_180_P, res.y * rot_angle * DX::Math::_180_P, res.z * rot_angle * DX::Math::_180_P };
}

DirectX::XMFLOAT3 DX::Transform::GetScale() const {
	DirectX::XMVECTOR t_pos, t_rot, scale;
	DirectX::XMFLOAT3 res;

	DirectX::XMMatrixDecompose(&scale, &t_rot, &t_pos, m_transformation);
	DirectX::XMStoreFloat3(&res, scale);

	return res;
}

void DX::Transform::Translate(DirectX::XMFLOAT3 _offset) {
	m_transformation = DirectX::XMMatrixMultiply(
		m_transformation,
		DirectX::XMMatrixTranslation(_offset.x, _offset.y, _offset.z)
	);
}

void DX::Transform::Rotate(DirectX::XMFLOAT3 _axis, float _angleEuler) {
	m_transformation = DirectX::XMMatrixMultiply(
		DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&_axis), _angleEuler * DX::Math::PI_180),
		m_transformation
	);
}

void DX::Transform::Scale(DirectX::XMFLOAT3 _axis) {
	m_transformation = DirectX::XMMatrixMultiply(
		DirectX::XMMatrixScaling(_axis.x, _axis.y, _axis.z),
		m_transformation
	);
}