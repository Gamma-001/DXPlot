#pragma once

#include <Engine.hpp>
#include <GUI/Window.hpp>

class MainHandler : public Cass::WndEventHandler {
public:
	LRESULT OnSize(HWND _hWnd, WPARAM _wParam, LPARAM _lParam);
};

void HandleNavigation(HWND _hWnd);