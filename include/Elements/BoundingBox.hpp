#pragma once

#include <Elements/Ray.hpp>

#include <DirectXMath.h>

namespace Cass {
	/**
	* Represents an axis aligned 3D bounding box, with a center of mass (m_pos) and dimensions on each axis (m_dims)
	* All 3D objects must include a single bounding box
	**/
	class BoundingBox {
	public:
		BoundingBox(DirectX::XMFLOAT3 _pos = { 0.0f, 0.0f, 0.0f }, DirectX::XMFLOAT3 _dims = { 0.0f, 0.0f, 0.0f });
		
		DirectX::XMFLOAT3 GetPosition() const { return m_pos; }
		DirectX::XMFLOAT3 GetDimensions() const { return m_dims; }
		
		void Update(DirectX::XMFLOAT3 _pos, DirectX::XMFLOAT3 _dims);

		/**
		* @brief Calculates bounding box from lower and upper bounds on each axis
		* @param _lb per axis lower bound
		* @param _ub per axis upper bound 
		*/
		void Calculate(DirectX::XMFLOAT3 _lb, DirectX::XMFLOAT3 _ub);

		/**
		* @brief Calculates ray-box intersection
		* @return 1: front facing intersection, -1: back facing intersection, 0: no intersection
		*/
		int Intersect(const Cass::Ray& _ray) const;

	private:
		DirectX::XMFLOAT3 m_pos;
		DirectX::XMFLOAT3 m_dims;
	};
}