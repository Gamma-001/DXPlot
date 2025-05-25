#pragma once

#include <Transform.hpp>
#include <Object/Empty.hpp>
#include <Resource/Shader.hpp>
#include <Object/Camera.hpp>
#include <Object/Empty.hpp>

#include <d3d11.h>
#include <WRL/client.h>
#include <DirectXMath.h>

#include <numeric>
#include <cmath>
#include <vector>

namespace Cass {
	namespace detail {
		struct MESH_VERTEX_DATA {
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT3 normal;
			DirectX::XMFLOAT2 uv;
		};
	}

	enum class SHADING {
		FLAT,
		SMOOTH
	};

	class Mesh : public Transform {
	public:
		Mesh(size_t _vertCount, size_t _polyCount, SHADING _shading, ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);

		virtual ~Mesh();

		/**
		* @brief Renders the mesh if buffer size is valid
		*/
		virtual void Render(Camera& _camera, Shader& _shader);

		/**
		* @brief Duplicate per face normals on connected vertices
		*
		* @param polyCount		total tri count
		* @param indices		vertex indices per face
		* @param faceNormals	per face normals
		* @param normals		unallocated FLOAT3 array
		*/
		static void SetSplitNormals(_In_ size_t polyCount, _In_ uint32_t* indices, _In_ DirectX::XMFLOAT3* faceNormals, _Out_ std::unique_ptr <DirectX::XMFLOAT3[]>& normals);

		/**
		* @brief Calculate per vertex normals by averaging face normals
		*
		* @param polyCount		total tri count
		* @param vertCount		total vertex count
		* @param indices		vertex indices per face
		* @param faceNormals	per face normals
		* @param normals		unallocated FLOAT3 array
		*/
		static void SetSmoothNormals(_In_ size_t polyCount, _In_ size_t vertCount, _In_ uint32_t* indices, _In_ DirectX::XMFLOAT3* faceNormals, _Out_ std::unique_ptr <DirectX::XMFLOAT3[]>& normals);

		void CalculateNormalsFromFace();

		void GetPositions(std::vector <DirectX::XMFLOAT3> &_oPos) const;
		void GetNormals(std::vector <DirectX::XMFLOAT3> &_oNorm) const;
		void GetUVs(std::vector <DirectX::XMFLOAT2> &_oUV) const;

		void SetPositions(const std::vector<DirectX::XMFLOAT3> &_position);
		
		void ShowBounds(bool _toggle);

	protected:
		/** 
		* @brief Initialize vertices, normals, UV, vertex colors and indices, for primitives
		*/
		virtual void InitVertices() = 0;
		
		/**
		* @brief Set smooth or flat shading by modifying vertex data (requires indices to be set assuming smooth shading prior to call)
		*		 should only be called on initialization
		* 
		* @param _pFaceNormals Per face normals for each triangle
		*/
		void SetShading(DirectX::XMFLOAT3* _pFaceNormals);

		/**
		* @brief Copy vertex data into vertex and index buffer
		*/
		void SetBuffers();

		/**
		* @brief Create vertex and index buffers based on vertex and face count
		*/
		void CreateBuffers();

		size_t m_vertCount, m_polyCount;
		SHADING m_shadingMode;

		std::unique_ptr <uint32_t[]> m_indices;
		std::unique_ptr <detail::MESH_VERTEX_DATA[]> m_vertexData;
		Microsoft::WRL::ComPtr <ID3D11Buffer> m_vBuffer;
		Microsoft::WRL::ComPtr <ID3D11Buffer> m_iBuffer;
	
		Microsoft::WRL::ComPtr <ID3D11Device> m_device;
		Microsoft::WRL::ComPtr <ID3D11DeviceContext> m_deviceContext;

	private:
		std::unique_ptr <Box> m_boundsMesh;
		static std::unique_ptr <FlatShader> s_defShader;
	};

	class RegularPolygon : public Mesh {
	public:
		RegularPolygon(
			float _radius, uint32_t _degree,
			ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext,
			SHADING _shading = SHADING::SMOOTH);

	protected:
		void InitVertices() override;

	private:
		float m_radius;
		uint32_t m_degree;
	};

	class Cuboid : public Mesh {
	public:
		Cuboid(
			float _width, float _height, float _depth,
			ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext,
			SHADING _shading = SHADING::FLAT);

	protected:
		void InitVertices() override;

	private:
		float m_width, m_height, m_depth;
	};

	class Sphere : public Mesh {
	public:
		Sphere(
			float _radius, uint32_t _resX, uint32_t _resY,
			ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext,
			SHADING _shading = SHADING::SMOOTH
		);

	protected:
		void InitVertices() override;

	private:
		float m_radius;
		uint32_t m_resX, m_resY;
	};

	class Plane : public Mesh {
	public:
		Plane(
			float _width, float _length, uint32_t _resX, uint32_t _resY,
			ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext,
			SHADING _shading = SHADING::SMOOTH
		);

	protected:
		void InitVertices() override;

	private:
		float m_width, m_length;
		uint32_t m_resX, m_resY;
	};

	class CustomMesh : public Mesh {
	public:
		CustomMesh(ID3D11Device*, ID3D11DeviceContext*);

		/**
		* @brief Load a single mesh from given file
		* @param _log if not nullptr, must be allocated with large enough capacity
		*/
		HRESULT LoadFromFile(_In_ std::string _fName, _In_ ID3D11Device* _pDevice, _In_ ID3D11DeviceContext* _pContext, _Out_opt_ char *_log = nullptr);
	
	protected:
		void InitVertices() override;
	};
}