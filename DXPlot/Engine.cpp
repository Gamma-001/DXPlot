#include <Engine.hpp>

#include <DirectXMath.h>
#include <exception>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND Application::Window::s_hWnd = NULL;
std::pair <int, int> Application::Window::s_dimensions = std::make_pair(0, 0);

template class Application::MeshObject <DX::Mesh>;
template class Application::MeshObject <DX::Plane>;

//
// ---------- Window
//

bool Application::Window::Initialize(HINSTANCE hInst, LPCWSTR title, int width, int height) {
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

	DX::ThrowIfFailed(CoInitializeEx(NULL, COINIT_MULTITHREADED));

	return true;
}

bool Application::Window::SetHandler(EventHandler* handler) {
	if (s_hWnd == NULL) return false;

	return SetWindowLongPtr(s_hWnd, GWLP_USERDATA, reinterpret_cast <LONG_PTR> (handler)) != NULL;
}

void Application::Window::Destroy(HINSTANCE hInst) {
	if (s_hWnd != NULL) {
		DestroyWindow(s_hWnd);
		UnregisterClassW(L"MainWindow", hInst);
		s_hWnd = NULL;
	}
}

LRESULT CALLBACK Application::Window::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	switch (uMsg) {

	// mouse input

	case WM_LBUTTONDOWN:
		DX::Mouse::pressedLB = true;
		return 0;

	case WM_LBUTTONUP:
		DX::Mouse::pressedLB = false;
		return 0;

	case WM_MBUTTONDOWN:
		DX::Mouse::pressedMB = true;
		return 0;

	case WM_MBUTTONUP:
		DX::Mouse::pressedMB = false;
		return 0;

	case WM_MOUSEWHEEL:
		DX::Mouse::wheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
		return 0;

	// keyboard messages

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_SHIFT:
			DX::Keyboard::controls.l_shift = true;
			break;
		}
		return 0;

	case WM_KEYUP:
		switch (wParam) {
		case VK_SHIFT:
			DX::Keyboard::controls.l_shift = false;
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
		Application::EventHandler* handler = reinterpret_cast <Application::EventHandler*> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
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

//
// ---------- D3DScene
//

std::shared_ptr <DX::SurfaceShader> Application::D3DScene::s_defSurf = nullptr;

Application::D3DScene::D3DScene() {
	if (!DirectX::XMVerifyCPUSupport())
		throw std::exception("DirectX 11 Math not supported by current CPU");

	m_grid.push_back(EmptyObject("XYgrid0")); m_grid.push_back(EmptyObject("XYgrid1"));
	m_axis = EmptyObject("Axis");

	m_camera = DX::Camera(DirectX::XMFLOAT3(0.0f, 0.0f, -10.0f));
	m_camera.Rotate(DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f), -60.0f);
	m_camera.Rotate(DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), 45.0f);

	m_msaa = true;
	m_showGrid = true;
}

void Application::D3DScene::CreateD3DViewport(Window _window, D3D_FEATURE_LEVEL _minFeatureLevel, bool _msaa) {
	if (_window.GetHandle() == NULL)		throw std::exception("Invalid arguments");

	if (_msaa)	m_resources = DX::DeviceResources(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, 1U, _minFeatureLevel);
	else		m_resources = DX::DeviceResources(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, 1U, _minFeatureLevel, false);

	RECT rect;
	GetClientRect(_window.GetHandle(), &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	m_resources.CreateDeviceResource(D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED);
	m_resources.SetWindow(_window.GetHandle(), width, height);
	m_resources.CreateSizeDependentResource();
	m_resources.SetViewport();

	if (s_defSurf.get() == nullptr) {
		std::shared_ptr <DX::Texture> tex = std::make_shared <DX::Texture> (D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);
		DX::ThrowIfFailed(tex->CreateFromSolidColor(16, 16, 255, 255, 255, 255, m_resources.GetDevice(), m_resources.GetDeviceContext()));

		s_defSurf = std::make_shared <DX::SurfaceShader>(tex, DirectX::XMFLOAT4 { 0.8f, 0.8f, 0.8f, 1.0f });
		DX::ThrowIfFailed(s_defSurf->LoadFromFile(L"../shaders/defLitShader.hlsl", m_resources.GetDevice(), m_resources.GetDeviceContext()));
		
		tex.reset();
	}
	std::shared_ptr <DX::FlatShader> gridShader1 = std::make_shared <DX::FlatShader>();
	std::shared_ptr <DX::FlatShader> gridShader2 = std::make_shared <DX::FlatShader>();
	std::shared_ptr <DX::FlatShader> axisShader = std::make_shared <DX::FlatShader>();

	DX::ThrowIfFailed(gridShader1->LoadFromFile(L"../shaders/axisGridShader.hlsl", m_resources.GetDevice(), m_resources.GetDeviceContext()));
	DX::ThrowIfFailed(gridShader2->LoadFromFile(L"../shaders/axisGridShader.hlsl", m_resources.GetDevice(), m_resources.GetDeviceContext()));
	DX::ThrowIfFailed(axisShader->LoadFromFile(L"../shaders/axisGridShader.hlsl", m_resources.GetDevice(), m_resources.GetDeviceContext()));

	m_camera.SetProjection(DX::PROJECTION::PERSPECTIVE, static_cast <float> (width), static_cast <float> (height), 0.1f, 1000.0f, 70.0f);
	m_grid[0].pEmpty = std::make_unique <DX::Grid> (1000.0f, 1000.0f, 999, 999, 10, DirectX::XMFLOAT4 { 0.8f, 0.8f, 0.8f, 0.4f }, m_resources.GetDevice(), m_resources.GetDeviceContext());
	m_grid[0].pShader = gridShader1;
	m_grid[1].pEmpty = std::make_unique <DX::Grid>(1000.0f, 1000.0f, 99, 99, 50, DirectX::XMFLOAT4{ 0.8f, 0.8f, 0.8f, 0.4f }, m_resources.GetDevice(), m_resources.GetDeviceContext());
	m_grid[1].pShader = gridShader2;

	m_axis.pEmpty = std::make_unique <DX::Grid>(1000.0f, 1000.0f, 1, 1, 10, DirectX::XMFLOAT4 { 1.0f, 1.0f, 1.0f, 1.0f}, m_resources.GetDevice(), m_resources.GetDeviceContext());
	m_axis.pShader = axisShader;
	m_axis.pEmpty->Translate({ 0.0f, 0.0f, 0.005f });
	
	m_axis.pEmpty->SetColor(0, { 0.2f, 1.0f, 0.2f, 0.7f });
	m_axis.pEmpty->SetColor(1, { 0.2f, 1.0f, 0.2f, 0.7f });
	m_axis.pEmpty->SetColor(2, { 1.0f, 0.2f, 0.2f, 0.7f });
	m_axis.pEmpty->SetColor(3, { 1.0f, 0.2f, 0.2f, 0.7f });

	m_axis.pEmpty->SetBuffers(m_resources.GetDeviceContext());

	m_msaa = _msaa;
}

void Application::D3DScene::Render(const float _clearColor[4], int _syncInterval, bool _msaa) {
	if (m_resources.GetDevice() == nullptr) return;

	m_resources.Clear(_clearColor);

	// draw meshes without culling
	if (m_msaa && _msaa) m_resources.SetRenderTarget_msaa(false);
	else m_resources.SetRenderTarget_no_msaa(false);
	for (auto& mesh : m_vec_mesh) {
		if (!mesh->culling)
			mesh->pMesh->Render(m_camera, m_resources.GetDeviceContext(), *mesh->pShader.get());
	}
	for (auto& plane : m_vec_plane) {
		if (!plane->culling)
			plane->pMesh->Render(m_camera, m_resources.GetDeviceContext(), *plane->pShader.get());
	}

	// draw meshes with culling
	if (m_msaa && _msaa) m_resources.SetRenderTarget_msaa();
	else m_resources.SetRenderTarget_no_msaa();
	for (auto& mesh : m_vec_mesh) {
		if (mesh->culling)
			mesh->pMesh->Render(m_camera, m_resources.GetDeviceContext(), *mesh->pShader.get());
	}
	for (auto& plane : m_vec_plane) {
		if (plane->culling) 
			plane->pMesh->Render(m_camera, m_resources.GetDeviceContext(), *plane->pShader.get());
	}

	// draw emptys
	if (m_msaa && _msaa) m_resources.SetRenderTarget_msaa(true, true, true);
	if (m_showGrid) {
		m_grid[0].pShader->m_color.w = std::min(m_camera.GetScale().x / 2.0f, 1.0f);

		m_grid[0].pEmpty->Render(m_camera, m_resources.GetDeviceContext(), *m_grid[0].pShader);
		m_grid[1].pEmpty->Render(m_camera, m_resources.GetDeviceContext(), *m_grid[1].pShader);
		
		m_axis.pEmpty->Render(m_camera, m_resources.GetDeviceContext(), *m_axis.pShader);
	}

	// resolve onto non msaa render target, if msaa is enabled
	if (m_msaa && _msaa) {
		m_resources.Resolve();
		m_resources.SetRenderTarget_no_msaa();
	}

	m_resources.GetSwapChain()->Present(_syncInterval, NULL);
}

void Application::D3DScene::ResizeContext(int _width, int _height) {
	m_resources.WindowSizeChanged(_width, _height);
	m_resources.SetViewport();
	m_camera.SetProjection(DX::PROJECTION::PERSPECTIVE, static_cast <float> (_width), static_cast <float> (_height), 0.1f, 1000.0f, 70.0f);
}

// Object Creation

void Application::D3DScene::AddPolygon(const std::string& _name, float _radius, uint32_t _degree, DX::SHADING _shading, bool _culling) {
	if (m_resources.GetDevice() == nullptr || m_resources.GetDeviceContext() == nullptr) {
		throw std::invalid_argument("Device or device context not created");
	}

	std::unique_ptr<MeshObject <DX::Mesh>> mesh = std::make_unique<MeshObject <DX::Mesh>>(_name);
	mesh->pMesh = std::make_unique <DX::RegularPolygon> (_radius, _degree, m_resources.GetDevice(), m_resources.GetDeviceContext(), _shading);
	mesh->pShader = s_defSurf;
	mesh->culling = _culling;
	m_vec_mesh.push_back(std::move(mesh));
}

void Application::D3DScene::AddCuboid(const std::string& _name, float _width, float _height, float _depth, DX::SHADING _shading, bool _culling) {
	if (m_resources.GetDevice() == nullptr || m_resources.GetDeviceContext() == nullptr) {
		throw std::invalid_argument("Device or device context not created");
	}

	std::unique_ptr<MeshObject <DX::Mesh>> mesh = std::make_unique<MeshObject <DX::Mesh>>(_name);
	mesh->pMesh = std::make_unique <DX::Cuboid> (_width, _height, _depth, m_resources.GetDevice(), m_resources.GetDeviceContext(), _shading);
	mesh->pShader = s_defSurf;
	mesh->culling = _culling;
	m_vec_mesh.push_back(std::move(mesh));
}

void Application::D3DScene::AddSphere(const std::string& _name, float _radius, uint32_t _resX, uint32_t _resY, DX::SHADING _shading, bool _culling) {
	if (m_resources.GetDevice() == nullptr || m_resources.GetDeviceContext() == nullptr) {
		throw std::invalid_argument("Device or device context not created");
	}

	std::unique_ptr<MeshObject <DX::Mesh>> mesh = std::make_unique<MeshObject <DX::Mesh>>(_name);
	mesh->pMesh = std::make_unique <DX::Sphere> (_radius, _resX, _resY, m_resources.GetDevice(), m_resources.GetDeviceContext(), _shading);
	mesh->pShader = s_defSurf;
	mesh->culling = _culling;
	m_vec_mesh.push_back(std::move(mesh));
}

void Application::D3DScene::AddPlane(const std::string& _name, float _width, float _length, uint32_t _resX, uint32_t _resY, DX::SHADING _shading, bool _culling) {
	if (m_resources.GetDevice() == nullptr || m_resources.GetDeviceContext() == nullptr) {
		throw std::invalid_argument("Device or device context not created");
	}

	std::unique_ptr<MeshObject <DX::Plane>> mesh = std::make_unique<MeshObject <DX::Plane>>(_name);
	mesh->pMesh = std::make_unique <DX::Plane> (_width, _length, _resX, _resY, m_resources.GetDevice(), m_resources.GetDeviceContext(), _shading);
	mesh->pShader = s_defSurf;
	mesh->culling = _culling;
	m_vec_plane.push_back(std::move(mesh));
}