#pragma once

#include <DirectXMath.h>

namespace Cass {
	/**
	* Represents a ray in 3D
	* composed of point of emergence (m_origin) and a normalized dimension vector (m_dir)
	*/
	class Ray {
	public:
		Ray(DirectX::XMFLOAT3 _origin = { 0.0f, 0.0f, 0.0f }, DirectX::XMFLOAT3 _dir = { 0.0f, 0.0f, 0.0f });
		
		void Update(DirectX::XMFLOAT3 _origin, DirectX::XMFLOAT3 _dir);

		/**
		* @brief Calculate if the ray intersects a triangle
		* @return true on intersection
		*/
		bool IntersectTriangle(DirectX::XMFLOAT3 _a, DirectX::XMFLOAT3 _b, DirectX::XMFLOAT3 _c) const;
	private:
		DirectX::XMFLOAT3 m_origin;
		DirectX::XMFLOAT3 m_dir;
	};
}