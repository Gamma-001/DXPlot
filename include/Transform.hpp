#pragma once

#include <DirectXMath.h>
#include <Elements/BoundingBox.hpp>

namespace Cass {
	enum class AXIS {
		X,
		Y,
		Z
	};

	class Transform {
	public:
		Transform();

		DirectX::XMFLOAT3 GetPosition() const;
		DirectX::XMFLOAT4 GetRotationQuat() const;
		DirectX::XMFLOAT3 GetRotationEuler() const;
		DirectX::XMFLOAT3 GetScale() const;
		BoundingBox GetBounds() const { return m_bounds; }
		
		void ResetTransform();
		void Translate(DirectX::XMFLOAT3 _offset);
		void Rotate(DirectX::XMFLOAT3 _axis, float _angleEuler);
		void Scale(DirectX::XMFLOAT3 _axis);

		/**
		* @brief determine whether there is an intersection between a given ray and the current bounding box
		* @returns 1: front face intersection, 0: no intersections, -1: only backfacing intersection found
		*/
		int IntersectBox(const Ray& ray) const;

		virtual ~Transform();

	protected:
		DirectX::XMMATRIX m_transformation;
		BoundingBox m_bounds;
	};
}