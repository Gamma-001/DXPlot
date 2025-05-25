#include <Object/Camera.hpp>
#include <util.hpp>

using namespace Cass;

//
// public methods
//

Camera::Camera(DirectX::XMFLOAT3 _position) {
	m_target = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);

	m_viewMat = DirectX::XMMatrixTranslation(-_position.x, -_position.y, -_position.z);
	m_projectionMat = DirectX::XMMatrixIdentity();
}

void Camera::SetProjection(PROJECTION _projection, float _width, float _height, float _nearZ, float _farZ, float _fov) {
	switch (_projection) {
	case PROJECTION::PERSPECTIVE:
		m_projectionMat = DirectX::XMMatrixPerspectiveFovLH(Math::PI_180 * _fov, _width / _height, _nearZ, _farZ);
		break;
	case PROJECTION::ORTHOGRAPHIC:
		m_projectionMat = DirectX::XMMatrixOrthographicLH(_width, _height, _nearZ, _farZ);
		break;
	default:
		return;
	}
}

// getters

DirectX::XMMATRIX Camera::GetViewMat() const {
	return DirectX::XMMatrixMultiply(
		DirectX::XMMatrixTranslation(m_target.x, m_target.y, m_target.z),
		m_viewMat
	);
}

DirectX::XMFLOAT3 Camera::GetFrontDir() const { return GetLocalDir({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }); }
DirectX::XMFLOAT3 Camera::GetRightDir() const { return GetLocalDir({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }); }
DirectX::XMFLOAT3 Camera::GetUpDir()	const { return GetLocalDir({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }); }

// transformations

void Camera::TranslateTarget(DirectX::XMFLOAT3 _offset) {
	m_target = { m_target.x - _offset.x, m_target.y - _offset.y, m_target.z - _offset.z };
}

void Camera::TranslateTargetLocal(AXIS _axis, float _offset) {
	static DirectX::XMFLOAT3 dirs[] = {
		{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}
	};
	auto dir = GetLocalDir({ 0.0f, 0.0f, 0.0f }, dirs[static_cast <int> (_axis)]);
	m_target = { m_target.x - _offset * dir.x, m_target.y - _offset * dir.y, m_target.z - _offset * dir.z };
}

void Camera::Translate(DirectX::XMFLOAT3 _offset) {
	m_viewMat = DirectX::XMMatrixMultiply(
		m_viewMat,
		DirectX::XMMatrixTranslation(-_offset.x, -_offset.y, -_offset.z)
	);
}

void Camera::RotateRelative(DirectX::XMFLOAT3 _axis, float _angle) {
	float theta = -_angle * Math::PI_180;
	float cx = cos(theta / 2), sx = sin(theta / 2);

	DirectX::XMFLOAT4 q_axis = { sx * _axis.x, sx * _axis.y, sx * _axis.z, cx };
	m_viewMat = DirectX::XMMatrixMultiply(
		m_viewMat,
		DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&q_axis))
	);
}

void Camera::Rotate(DirectX::XMFLOAT3 _axis, float _angle) {
	float theta = -_angle * Math::PI_180;
	float cx = cos(theta / 2), sx = sin(theta / 2);

	DirectX::XMFLOAT4 q_axis = { sx * _axis.x, sx * _axis.y, sx * _axis.z, cx };
	m_viewMat = DirectX::XMMatrixMultiply(
		DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&q_axis)),
		m_viewMat
	);
}

void Camera::RotateXY(float _angle) {
	// cancel all the rotation, then apply them back after applying the supplied rotation
	DirectX::XMVECTOR t_vec, r_vec, s_vec;
	DirectX::XMFLOAT4 rot;

	DirectX::XMMatrixDecompose(&s_vec, &r_vec, &t_vec, m_viewMat);
	DirectX::XMStoreFloat4(&rot, r_vec);
	rot.x *= -1; rot.y *= -1; rot.z *= -1;
	m_viewMat = DirectX::XMMatrixMultiply(
		DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rot)),
		m_viewMat
	);

	Rotate({ 1.0f, 0.0f, 0.0f }, _angle);

	rot.x *= -1; rot.y *= -1; rot.z *= -1;
	m_viewMat = DirectX::XMMatrixMultiply(
		DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rot)),
		m_viewMat
	);
}

void Camera::Scale(float _amount, DirectX::XMFLOAT3 _axis) {
	m_scale.x *= _amount * _axis.x;
	m_scale.y *= _amount * _axis.y;
	m_scale.z *= _amount * _axis.z;

	m_viewMat = DirectX::XMMatrixMultiply(
		DirectX::XMMatrixScaling(_amount * _axis.x, _amount * _axis.y, _amount * _axis.z),
		m_viewMat
	);
}

//
// private methods
//

void Camera::ApplyTransform(DirectX::XMFLOAT3& _pos) const {
	// view matrix is stored as column major for use as shader resource
	// for any transformation on the CPU it needs to be transposed first
	DirectX::XMStoreFloat3(
		&_pos,
		DirectX::XMVector4Transform(DirectX::XMLoadFloat3(&_pos), DirectX::XMMatrixTranspose(m_viewMat))
	);
}

DirectX::XMFLOAT3 Camera::GetLocalDir(DirectX::XMFLOAT3 _a, DirectX::XMFLOAT3 _b) const {
	// applying the transform to a vector in global coordinates will yield a direction in local coordinates

	ApplyTransform(_a);
	ApplyTransform(_b);

	// normalize
	DirectX::XMFLOAT3 res;
	DirectX::XMFLOAT3 dir = { _b.x - _a.x, _b.y - _a.y, _b.z - _a.z };
	DirectX::XMStoreFloat3(
		&res,
		DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&dir))
	);
	return res;
}