#pragma once

#include <DirectXMath.h>

namespace DX {
	class Transform {
	public:
		Transform();

		DirectX::XMFLOAT3 GetPosition() const;
		DirectX::XMFLOAT4 GetRotationQuat() const;
		DirectX::XMFLOAT3 GetRotationEuler() const;
		DirectX::XMFLOAT3 GetScale() const;

		void Translate(DirectX::XMFLOAT3 _offset);
		void Rotate(DirectX::XMFLOAT3 _axis, float _angleEuler);
		void Scale(DirectX::XMFLOAT3 _axis);

		virtual ~Transform();

	protected:
		DirectX::XMMATRIX m_transformation;
	};
}