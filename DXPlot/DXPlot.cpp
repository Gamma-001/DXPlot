#include <DXPlot.hpp>

Application::D3DScene g_scene;

int APIENTRY wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInst, _In_ LPWSTR cmdLine, _In_ int nCmdShow) {
	Application::Window mainWindow;

	if (!mainWindow.Initialize(hInst, L"DXPlot")) {
		OutputDebugString(L"::ERROR::\tFailed to create a window\n");
		return -1;
	}

	HWND hWnd = mainWindow.GetHandle();
	g_scene.CreateD3DViewport(mainWindow, D3D_FEATURE_LEVEL_11_0, true);
	ShowWindow(hWnd, nCmdShow);

	MainHandler handler;
	mainWindow.SetHandler(static_cast <Application::EventHandler*> (&handler));

	// plotting
	g_scene.AddPlane("plane", 4.0f, 4.0f, 128, 128, DX::SHADING::SMOOTH);
	g_scene.DisplacePlane(0,
		[](float x, float y) {
			return 0.2f * sin((pow(x, 2) + pow(y, 2)) * 3.0f);
		}, 50.0f, true
	);

	bool terminate = false;
	float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	while (true) {
		MSG msg;
		while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				terminate = true;
				break;
			}
		}
		if (terminate) break;

		HandleNavigation(hWnd);
		g_scene.Render(clearColor, 1, true);
	}

	mainWindow.Destroy(hInst);

	return 0;
}

LRESULT MainHandler::OnSize(HWND _hWnd, WPARAM _wParam, LPARAM _lParam) {
	int n_width = LOWORD(_lParam);
	int n_height = HIWORD(_lParam);

	// size lower limit
	if (n_width < 100 || n_height < 100)
		return SendMessage(_hWnd, WM_SIZE, NULL, MAKELPARAM(100, 100));

	g_scene.ResizeContext(n_width, n_height);
	return 0;
}

void HandleNavigation(HWND _hWnd) {
	POINT mousePos;
	if (!GetCursorPos(&mousePos)) return;
	float diffX = static_cast <float> (DX::Mouse::posX - mousePos.x) / g_scene.GetWidth();
	float diffY = static_cast <float> (DX::Mouse::posY - mousePos.y) / g_scene.GetHeight();

	// wrap cursor movement around the screen for smooth navigation
	RECT clientRect = DX::GetAbsoluteClientRect(_hWnd);
	if (( DX::Mouse::pressedMB ) && 
		( mousePos.x >= clientRect.right - 5 || mousePos.x <= clientRect.left + 5 || mousePos.y <= clientRect.top + 5 || mousePos.y >= clientRect.bottom - 5 )) {
		if (mousePos.x >= clientRect.right - 5) {
			SetCursorPos(clientRect.left, mousePos.y);
			mousePos.x = clientRect.left;
		}
		else if (mousePos.x <= clientRect.left + 5) {
			SetCursorPos(clientRect.right, mousePos.y);
			mousePos.x = clientRect.right;
		}
		if (mousePos.y >= clientRect.bottom - 5) {
			SetCursorPos(mousePos.x, clientRect.top);
			mousePos.y = clientRect.top;
		}
		else if (mousePos.y <= clientRect.top + 5) {
			SetCursorPos(mousePos.x, clientRect.bottom);
			mousePos.y = clientRect.bottom;
		}
	}

	// translate view [Lshift + middle mouse button + drag]
	else if (DX::Mouse::pressedMB && DX::Keyboard::controls.l_shift) {
		g_scene.m_camera.TranslateTargetLocal(DX::AXIS::RIGHT, 5.0f * diffX / sqrt(g_scene.m_camera.GetScale().x));
		g_scene.m_camera.TranslateTargetLocal(DX::AXIS::FRONT, -5.0f * diffY / sqrt(g_scene.m_camera.GetScale().x));
	}

	// rotate view [middle mouse button + drag]
	else if (DX::Mouse::pressedMB && !DX::Keyboard::controls.l_shift) {
		g_scene.m_camera.RotateXY(-diffY * 150.0f);
		g_scene.m_camera.Rotate(DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), 150.0f * diffX);
	}

	// scale view [scroll wheel]
	if (DX::Mouse::wheelDelta != 0) {
		float scaleAmount = 1.0f;
		if (DX::Mouse::wheelDelta > 0) scaleAmount = 1.0f + 0.2f * static_cast <float> (DX::Mouse::wheelDelta) / 120.0f;
		else scaleAmount = scaleAmount = 1.0f / (1.0f - 0.2f * static_cast <float> (DX::Mouse::wheelDelta) / 120.0f);
		
		g_scene.m_camera.Scale(scaleAmount);
		DX::Mouse::wheelDelta = 0;
	}

	DX::Mouse::posX = mousePos.x;
	DX::Mouse::posY = mousePos.y;
}