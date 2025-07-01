#include <GUI/Window.hpp>

#include <util.hpp>
#include <Device/Keyboard.hpp>
#include <Device/Mouse.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx11.h>

using namespace Cass;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND Window::s_hWnd = NULL;
std::pair <int, int> Window::s_dimensions = std::make_pair(0, 0);

bool Window::Initialize(HINSTANCE hInst, LPCWSTR title, int width, int height) {
	if (s_hWnd != NULL) return true;

	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hInstance = hInst;
	wc.lpszClassName = L"MainWindow";
	wc.lpfnWndProc = (WNDPROC)WndProc;

	if (!RegisterClassEx(&wc)) return false;

	s_hWnd = CreateWindowEx(NULL, L"MainWindow", title, WS_OVERLAPPEDWINDOW, 0, 0, width, height, NULL, NULL, hInst, NULL);
	if (s_hWnd == NULL) return false;

	s_dimensions.first = width;
	s_dimensions.second = height;

	Cass::ThrowIfFailed(CoInitializeEx(NULL, COINIT_MULTITHREADED));

	return true;
}

bool Window::SetHandler(Cass::WndEventHandler* handler) {
	if (s_hWnd == NULL) return false;

	return SetWindowLongPtr(s_hWnd, GWLP_USERDATA, reinterpret_cast <LONG_PTR> (handler)) != NULL;
}

void Window::Destroy(HINSTANCE hInst) {
	if (s_hWnd != NULL) {
		DestroyWindow(s_hWnd);
		UnregisterClassW(L"MainWindow", hInst);
		s_hWnd = NULL;
	}
}

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	switch (uMsg) {

		// mouse input

	case WM_LBUTTONDOWN:
		Cass::Mouse::pressedLB = true;
		return 0;

	case WM_LBUTTONUP:
		Cass::Mouse::pressedLB = false;
		return 0;

	case WM_MBUTTONDOWN:
		Cass::Mouse::pressedMB = true;
		return 0;

	case WM_MBUTTONUP:
		Cass::Mouse::pressedMB = false;
		return 0;

	case WM_MOUSEWHEEL:
		Cass::Mouse::wheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
		return 0;

		// keyboard messages

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_SHIFT:
			Cass::Keyboard::controls.l_shift = true;
			break;
		}
		return 0;

	case WM_KEYUP:
		switch (wParam) {
		case VK_SHIFT:
			Cass::Keyboard::controls.l_shift = false;
			break;
		}
		return 0;

		// state messages

	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO sizeInfo = reinterpret_cast <LPMINMAXINFO> (lParam);
		sizeInfo->ptMinTrackSize.x = 300;
		sizeInfo->ptMinTrackSize.y = 300;

		return 0;
	}

	case WM_SIZE:
	{
		auto handler = reinterpret_cast <Cass::WndEventHandler*> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (handler != nullptr) return handler->OnSize(hWnd, wParam, lParam);

		return 0;
	}

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_CLOSE:
		if (MessageBox(hWnd, L"Close Window?", L"", MB_YESNO) == IDYES) {
			DestroyWindow(hWnd);
		}
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}