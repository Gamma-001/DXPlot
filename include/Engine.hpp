#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GUI/Window.hpp>
#include <Resource/DeviceResources.hpp>
#include <Resource/Shader.hpp>
#include <Resource/Texture.hpp>
#include <Object/Mesh.hpp>
#include <Object/Empty.hpp>

#include <vector>
#include <memory>

namespace Application {
	class Object {
	public:
		Object();
		Object(const std::string& _name);

		std::string m_name;

	protected:
		uint64_t m_id;
	};

	class MeshObject: public Object {
	public:
		MeshObject();
		MeshObject(const std::string& _name);

		bool culling;
		std::unique_ptr <Cass::Mesh> pMesh;
		std::shared_ptr <Cass::Shader> pShader;
	};

	class EmptyObject: public Object {
	public:
		EmptyObject();
		EmptyObject(const std::string& _name);

		std::unique_ptr <Cass::Empty> pEmpty;
		std::shared_ptr <Cass::Shader> pShader;
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
		MeshObject* GetMesh(size_t _index) const {
			if (_index >= 0 && _index < m_vec_mesh.size()) return m_vec_mesh[_index].get();
			return nullptr;
		}


		// state change and creation

		void CreateD3DViewport(Cass::Window _window, D3D_FEATURE_LEVEL _minFeatureLevel = D3D_FEATURE_LEVEL_11_0, bool _msaa = true);
		void Render(const float _clearColor[4], int _syncInterval, bool _msaa);
		void ResizeContext(int _width, int _height);

		// Resource Creation

		void AddPolygon(const std::string &_name, float _radius = 1.0f , uint32_t _degree = 16, Cass::SHADING _shading = Cass::SHADING::FLAT, bool _culling = true);
		void AddCuboid(const std::string& _name, float _width = 2.0f, float _height = 2.0f, float _depth = 2.0f, Cass::SHADING _shading = Cass::SHADING::FLAT, bool _culling = true);
		void AddSphere(const std::string& _name, float _radius = 1.0f, uint32_t _resX = 32, uint32_t _resY = 16, Cass::SHADING _shading = Cass::SHADING::SMOOTH, bool _culling = true);
		void AddPlane(const std::string& _name, float _width = 2.0f, float _length = 2.0f, uint32_t _resX = 32, uint32_t _resY = 32, Cass::SHADING _shading = Cass::SHADING::SMOOTH, bool _culling = false);

		void AddTexture(D3D11_FILTER _filter, D3D11_TEXTURE_ADDRESS_MODE _mode, LPCWSTR _filename);
		void AddTexture(D3D11_FILTER _filter, D3D11_TEXTURE_ADDRESS_MODE _mode, uint32_t _width, uint32_t _height, const std::vector <uint8_t> &_colorData);

		// Overlays

		void ToggleBoundingBox(bool _value);

		Cass::Camera m_camera;

	private:
		Cass::DeviceResources m_resources;
		std::vector <std::unique_ptr<MeshObject>> m_vec_mesh;
		std::vector <std::unique_ptr<EmptyObject>> m_vec_empty;
		std::vector <std::shared_ptr<Cass::Texture>> m_textures;
		std::vector <EmptyObject> m_grid;
		EmptyObject m_axis;

		bool m_msaa;
		bool m_showGrid;

		static std::shared_ptr <Cass::SurfaceShader> s_defSurf;
		static std::shared_ptr <Cass::FlatShader> s_defFlat;
	};
}

// TODO Refine scene class, all meshes should be stored in a single vector