#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Resource/DeviceResources.hpp>
#include <Resource/Shader.hpp>
#include <Resource/Texture.hpp>
#include <Object/Mesh.hpp>
#include <Object/Empty.hpp>
#include <Device/Keyboard.hpp>
#include <Device/Mouse.hpp>
#include <util.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx11.h>

#include <DirectXMath.h>
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <utility>
#include <vector>
#include <memory>

namespace Application {
	template <class T>
	struct MeshObject {
		MeshObject() : name(""), culling(true) {}
		MeshObject(const std::string& _name) : name(_name), culling(true) {}

		std::string name;
		std::unique_ptr <T> pMesh;
		std::shared_ptr <DX::Shader> pShader;
		bool culling;
	};

	struct EmptyObject {
		EmptyObject() : name("") {}
		EmptyObject(const std::string& _name) : name(_name) {}

		std::string name;
		std::unique_ptr <DX::Empty> pEmpty;
		std::shared_ptr <DX::Shader> pShader;
	};

	// interface for window message / event handler
	class EventHandler {
	public:
		virtual LRESULT OnSize(HWND _hWnd, WPARAM _wParam, LPARAM _lParam) = 0;
	};

	// wrapper class for window handling
	class Window {
	public:
		static bool Initialize(HINSTANCE hInst, LPCWSTR title, int width = 800, int height = 800);
		static bool SetHandler(EventHandler* handler);
		static void Destroy(HINSTANCE hInst);

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		HWND GetHandle()						const	{ return s_hWnd; }
		std::pair <int, int> GetDimensions()	const	{ return s_dimensions; }

	private:
		static HWND s_hWnd;
		static std::pair <int, int> s_dimensions;
	};

	class D3DScene {
	public:
		D3DScene();

		// getters

		int GetHeight() const {
			RECT rc = m_resources.GetClientRect();
			return rc.bottom - rc.top;
		}
		int GetWidth() const {
			RECT rc = m_resources.GetClientRect();
			return rc.right - rc.left;
		}
		MeshObject<DX::Mesh>* GetMesh(size_t _index) const {
			if (_index >= 0 && _index < m_vec_mesh.size()) return m_vec_mesh[_index].get();
			return nullptr;
		}
		MeshObject<DX::Plane>* GetPlane(size_t _index) const {
			if (_index >= 0 && _index < m_vec_plane.size()) return m_vec_plane[_index].get();
			return nullptr;
		}

		// state change and creation

		void CreateD3DViewport(Window _window, D3D_FEATURE_LEVEL _minFeatureLevel = D3D_FEATURE_LEVEL_11_0, bool _msaa = true);
		void Render(const float _clearColor[4], int _syncInterval, bool _msaa);
		void ResizeContext(int _width, int _height);

		// Object Creation and manipulation

		void AddPolygon(const std::string &_name, float _radius = 1.0f , uint32_t _degree = 16, DX::SHADING _shading = DX::SHADING::FLAT, bool _culling = true);
		void AddCuboid(const std::string& _name, float _width = 2.0f, float _height = 2.0f, float _depth = 2.0f, DX::SHADING _shading = DX::SHADING::FLAT, bool _culling = true);
		void AddSphere(const std::string& _name, float _radius = 1.0f, uint32_t _resX = 32, uint32_t _resY = 16, DX::SHADING _shading = DX::SHADING::SMOOTH, bool _culling = true);
		void AddPlane(const std::string& _name, float _width = 2.0f, float _length = 2.0f, uint32_t _resX = 32, uint32_t _resY = 32, DX::SHADING _shading = DX::SHADING::SMOOTH, bool _culling = false);

		/**
		* Displace vertices on z axis defined by the a lambda or class with overloaded operator ()
		* func should take two parameters: float x, float y
		* func must return float
		*/
		template <class DisplaceFunc>
		void DisplacePlane(size_t _index, DisplaceFunc _func, float _clamp, bool _center = false) {
			if (_index >= 0 && _index < m_vec_plane.size()) {
				m_vec_plane[_index].get()->pMesh->Displace(m_resources.GetDeviceContext(), _func, _clamp, _center);
			}
		}

		// Shader creation / selection

		DX::Camera m_camera;

	private:
		DX::DeviceResources m_resources;
		std::vector <std::unique_ptr<MeshObject <DX::Mesh>>> m_vec_mesh;
		std::vector <std::unique_ptr<MeshObject <DX::Plane>>> m_vec_plane;
		std::vector <std::unique_ptr<EmptyObject>> m_vec_empty;
		std::vector <EmptyObject> m_grid;
		EmptyObject m_axis;

		bool m_msaa;
		bool m_showGrid;

		static std::shared_ptr <DX::SurfaceShader> s_defSurf;
		static std::shared_ptr <DX::FlatShader> s_defFlat;
	};
}