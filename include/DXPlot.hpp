#pragma once

#include <Engine.hpp>

class MainHandler : public Application::EventHandler {
public:
	LRESULT OnSize(HWND _hWnd, WPARAM _wParam, LPARAM _lParam);
};

void HandleNavigation(HWND _hWnd);