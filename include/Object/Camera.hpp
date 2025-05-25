#pragma once

#include <DirectXMath.h>
#include <Transform.hpp>

namespace Cass {
	enum class PROJECTION {
		NONE,
		PERSPECTIVE,
		ORTHOGRAPHIC
	};

	class Camera {
	public:
		DirectX::XMMATRIX GetViewMat() const;
		DirectX::XMMATRIX GetProjectionMat() const { return m_projectionMat; }
		DirectX::XMFLOAT3 GetFrontDir() const;
		DirectX::XMFLOAT3 GetRightDir() const;
		DirectX::XMFLOAT3 GetUpDir() const;
		DirectX::XMFLOAT3 GetScale() const { return m_scale; }

		Camera(DirectX::XMFLOAT3 _position = { 0.0f, 0.0f, 0.0f });

		/**
		* @brief Contruct a Projection matrix for the camera
		* @param nearZ and farZ must be greated than 0
		* @param fov field of view in degrees
		*/
		void SetProjection(PROJECTION _projection, float _width, float _height, float _nearZ = 0.1f, float _farZ = 1000.0f, float _fov = 60.0f);

		/**
		* @brief Translate the camera rig to a specified target, in world space
		*/
		void TranslateTarget(DirectX::XMFLOAT3 _offset);

		/**
		* @brief Translate the camera rig to a specified target, in local space
		*/
		void TranslateTargetLocal(AXIS _axis, float _offset);

		/**
		* @brief Permanently translate the camera rig on world space, further rotation and scaling will be offseted
		*/
		void Translate(DirectX::XMFLOAT3 _offset);

		/**
		* @brief Rotate the camera around the target
		* @param angle rotation angle in degrees
		*/
		void RotateRelative(DirectX::XMFLOAT3 _axis, float _angle);

		/**
		* @brief Rotate the camera around its own origin (when target is non zero, it'll appear that the camera is orbiting a target point)
		* @param angle rotation angle in degrees
		*/
		void Rotate(DirectX::XMFLOAT3 _axis, float _angle);

		/**
		* @brief Rotate the XY plane by given angle in degrees
		*/
		void RotateXY(float _angle);

		/**
		* @brief Scale the view in local space(wrt target)
		*/
		void Scale(float _amount, DirectX::XMFLOAT3 _axis = { 1.0f, 1.0f, 1.0f });

	private:
		void ApplyTransform(DirectX::XMFLOAT3& _pos) const;
		DirectX::XMFLOAT3 GetLocalDir(DirectX::XMFLOAT3 _a, DirectX::XMFLOAT3 _b) const;

		DirectX::XMFLOAT3 m_target;
		DirectX::XMFLOAT3 m_scale;

		DirectX::XMMATRIX m_viewMat;
		DirectX::XMMATRIX m_projectionMat;
	};
}