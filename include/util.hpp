#pragma once

#include <windows.h>
#include <cassert>
#include <exception>
#include <sstream>
#include <DirectXMath.h>

namespace Cass {
	namespace Math {
		constexpr float PI = 3.14159265f;
		constexpr float PIx2 = 2.0f * PI;
		constexpr float PI_180 = PI / 180.0f;
		constexpr float _180_P = 180.0f / PI;

		inline DirectX::XMFLOAT3 XMFloat3Subtract(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b) {
			return DirectX::XMFLOAT3 { b.x - a.x, b.y - a.y, b.z - a.z };
		}
		inline DirectX::XMFLOAT3 XMFloat3Add(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b) {
			return DirectX::XMFLOAT3{ a.x + b.x, a.y + b.y, a.z + b.z };
		}
	}

	class ComException : public std::exception {
	private:
		HRESULT result;

	public:
		ComException(HRESULT hr) noexcept : result(hr) { }
		const char* what() const override;
	};

	// throw a com exception if an HRESULT fails
	inline void ThrowIfFailed(HRESULT hr) {
		if (FAILED(hr)) {
			throw ComException(hr);
		}
	}

	// substitute for printf for debug output
	void DebugLog(const char* fmt, ...);

	// release an interface pointer
	template <class Interface>
	void SafeRelease(Interface** ppInterface) {
		if (*ppInterface) {
			(*ppInterface)->Release();
			*ppInterface = NULL;
		}
	}

	// returns the client rect of the window but converted to screen coordinates
	RECT GetAbsoluteClientRect(HWND hWnd);
}