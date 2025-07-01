#pragma once

#include <windows.h>
#include <utility>

namespace Cass {

	// Interface for window message / event handler
	class WndEventHandler {
	public:
		virtual LRESULT OnSize(HWND _hWnd, WPARAM _wParam, LPARAM _lParam) = 0;
	};

	// Wrapped around win32 window
	class Window {
	public:
		static bool Initialize(HINSTANCE hInst, LPCWSTR title, int width = 800, int height = 800);
		static bool SetHandler(WndEventHandler* handler);
		static void Destroy(HINSTANCE hInst);

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		HWND GetHandle()						const { return s_hWnd; }
		std::pair <int, int> GetDimensions()	const { return s_dimensions; }

	private:
		static HWND s_hWnd;
		static std::pair <int, int> s_dimensions;
	};
}