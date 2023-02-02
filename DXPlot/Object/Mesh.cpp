#pragma once
#pragma warning (disable: 26451)

#include <Object/Mesh.hpp>
#include <util.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <cmath>
#include <vector>

//
// ---------- class Mesh
//

DX::Mesh::Mesh(size_t _vertCount, size_t _polyCount, DX::SHADING _shading) {
	m_shadingMode = _shading;
	m_polyCount = _polyCount;
	m_vertCount	= m_shadingMode == DX::SHADING::SMOOTH ? _vertCount : m_polyCount * 3;
}

DX::Mesh::~Mesh() { }

void DX::Mesh::Render(DX::Camera& _camera, ID3D11DeviceContext* _pContext, DX::Shader& _shader) {
	if (m_vertCount < 3 || m_polyCount < 1) return;
	if (m_vBuffer.Get() == nullptr || m_iBuffer.Get() == nullptr) return;

	UINT strides = sizeof(detail::MESH_VERTEX_DATA);
	UINT offsets = 0;

	_shader.SetActive(_pContext, _camera, m_transformation);

	_pContext->IASetVertexBuffers(0, 1, m_vBuffer.GetAddressOf(), &strides, &offsets);
	_pContext->IASetIndexBuffer(m_iBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_pContext->DrawIndexed(m_polyCount * 3, 0, 0);
}

void DX::Mesh::SetShading(DirectX::XMFLOAT3* _pFaceNormals) {
	if (m_shadingMode == DX::SHADING::FLAT) {
		std::unique_ptr <DirectX::XMFLOAT3[]> tempPos(new DirectX::XMFLOAT3[m_vertCount]);
		std::unique_ptr <DirectX::XMFLOAT2[]> tempUV(new DirectX::XMFLOAT2[m_vertCount]);
		std::unique_ptr <DirectX::XMFLOAT3[]> tempNorm(nullptr);

		// extract the position from the indices directly, make indices sequential
		for (size_t i = 0; i < m_vertCount; i++) {
			tempPos[i] = m_vertexData[m_indices[i]].position;
			tempUV[i] = m_vertexData[m_indices[i]].uv;
		}
		for (size_t i = 0; i < m_vertCount; i++) {
			m_indices[i] = i;
			m_vertexData[i].position = tempPos[i];
			m_vertexData[i].uv = tempUV[i];
		}

		SetSplitNormals(m_polyCount, m_indices.get(), _pFaceNormals, tempNorm);
		for (size_t i = 0; i < m_vertCount; i++) {
			m_vertexData[i].normal = tempNorm[i];
		}
	}
	else {
		std::unique_ptr <DirectX::XMFLOAT3[]> tempNorm(new DirectX::XMFLOAT3[m_vertCount]);
		SetSmoothNormals(m_polyCount, m_vertCount, m_indices.get(), _pFaceNormals, tempNorm);

		for (size_t i = 0; i < m_vertCount; i++) {
			m_vertexData[i].normal = tempNorm[i];
		}
	}
}

void DX::Mesh::SetBuffers(ID3D11DeviceContext* _pContext) {
	// copy vertex data into vertex buffer
	D3D11_MAPPED_SUBRESOURCE ms;
	DX::ThrowIfFailed(_pContext->Map(m_vBuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms));
	memcpy(ms.pData, m_vertexData.get(), sizeof(detail::MESH_VERTEX_DATA) * m_vertCount);
	_pContext->Unmap(m_vBuffer.Get(), NULL);

	// copy index data into index buffer
	ZeroMemory(&ms, sizeof(ms));
	DX::ThrowIfFailed(_pContext->Map(m_iBuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms));
	memcpy(ms.pData, m_indices.get(), sizeof(uint32_t) * m_polyCount * 3);
	_pContext->Unmap(m_iBuffer.Get(), NULL);
}

void DX::Mesh::CreateBuffers(ID3D11Device* _pDevice) {
	assert(m_vertCount > 2);

	// create the vertex buffer
	D3D11_BUFFER_DESC v_bdc;
	ZeroMemory(&v_bdc, sizeof(v_bdc));

	v_bdc.Usage = D3D11_USAGE_DYNAMIC;
	v_bdc.ByteWidth = sizeof(detail::MESH_VERTEX_DATA) * m_vertCount;
	v_bdc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	v_bdc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	DX::ThrowIfFailed(_pDevice->CreateBuffer(&v_bdc, nullptr, m_vBuffer.ReleaseAndGetAddressOf()));

	// create the index buffer;
	D3D11_BUFFER_DESC i_bdc;
	ZeroMemory(&i_bdc, sizeof(i_bdc));

	i_bdc.Usage = D3D11_USAGE_DYNAMIC;
	i_bdc.ByteWidth = sizeof(uint32_t) * m_polyCount * 3;
	i_bdc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	i_bdc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	DX::ThrowIfFailed(_pDevice->CreateBuffer(&i_bdc, nullptr, m_iBuffer.ReleaseAndGetAddressOf()));
}

void DX::Mesh::CalculateNormalsFromFace(ID3D11DeviceContext* _pContext) {
	if (m_polyCount < 1) return;

	std::unique_ptr <DirectX::XMFLOAT3[]> faceNorm(new DirectX::XMFLOAT3[m_polyCount]);
	for (size_t i = 0; i < m_polyCount * 3; i += 3) {
		DirectX::XMFLOAT3 a, b;

		a = DX::Math::XMFloat3Subtract(m_vertexData[m_indices[i + 1]].position, m_vertexData[m_indices[i]].position);
		b = DX::Math::XMFloat3Subtract(m_vertexData[m_indices[i + 2]].position, m_vertexData[m_indices[i]].position);

		DirectX::XMStoreFloat3(
			&faceNorm[i / 3],
			DirectX::XMVector3Normalize(DirectX::XMVector3Cross(
				DirectX::XMLoadFloat3(&a),
				DirectX::XMLoadFloat3(&b)
			))
		);
	}

	SetShading(faceNorm.get());
	SetBuffers(_pContext);
}

// static methods

void DX::Mesh::SetSplitNormals(_In_ size_t polyCount, _In_ uint32_t* indices, _In_ DirectX::XMFLOAT3* faceNormals, _Out_ std::unique_ptr <DirectX::XMFLOAT3[]>& normals) {
	if (!polyCount || !indices || !faceNormals) return;

	normals = std::unique_ptr <DirectX::XMFLOAT3[]>(new DirectX::XMFLOAT3[static_cast <SIZE_T> (polyCount) * 3]);

	// copy normals on each vertex of face
	for (size_t i = 0; i < polyCount * 3; i += 3) {
		normals[indices[i]] = faceNormals[i / 3];
		normals[indices[i + 1]] = faceNormals[i / 3];
		normals[indices[i + 2]] = faceNormals[i / 3];
	}
}

void DX::Mesh::SetSmoothNormals(_In_ size_t polyCount, _In_ size_t vertCount, _In_ uint32_t* indices, _In_ DirectX::XMFLOAT3* faceNormals, _Out_ std::unique_ptr <DirectX::XMFLOAT3[]>& normals) {
	if (!polyCount || !vertCount || !indices || !faceNormals) return;

	normals.reset();
	normals = std::unique_ptr <DirectX::XMFLOAT3[]>(new DirectX::XMFLOAT3[vertCount]);
	std::vector <std::vector <uint32_t>> dict(vertCount, std::vector <uint32_t> {});

	// vertex to face mapping
	for (size_t i = 0; i < polyCount * 3; i += 3) {
		dict[indices[i]].push_back(i / 3);
		dict[indices[i + 1]].push_back(i / 3);
		dict[indices[i + 2]].push_back(i / 3);
	}

	// set vertex normals to the normalized sum of connected face normals
	for (size_t i = 0; i < vertCount; i++) {
		DirectX::XMFLOAT3 res = { 0, 0, 0 };
		for (auto& x : dict[i]) {
			res = DX::Math::XMFloat3Add(res, faceNormals[x]);
		}
		DirectX::XMStoreFloat3(
			&(normals[i]),
			DirectX::XMVector3Normalize(
				DirectX::XMLoadFloat3(&res)
			)
		);
	}
}

//
// ---------- class RegularPolygon
//

DX::RegularPolygon::RegularPolygon(
	float _radius, uint32_t _degree, 
	ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, 
	DX::SHADING _shading) : Mesh(static_cast <size_t> (_degree) + 1, _degree, _shading) {

	m_degree = _degree;
	m_radius = _radius;

	if (_degree < 3) return;

	CreateBuffers(_pDevice);
	InitVertices(_pContext);
}

void DX::RegularPolygon::InitVertices(ID3D11DeviceContext* _pContext) {
	m_vertexData = std::unique_ptr <detail::MESH_VERTEX_DATA[]> (new detail::MESH_VERTEX_DATA[m_vertCount]);
	m_indices = std::unique_ptr <uint32_t[]> (new uint32_t[m_polyCount * 3]);

	float theta = 2.0f * DX::Math::PI;
	float offset = 2.0f * DX::Math::PI / m_degree;

	m_vertexData[0].position = { 0.0f, 0.0f, 0.0f };
	m_vertexData[0].uv = { 0.5f, 0.5f };

	// winding order : CW
	for (UINT i = 1; i < m_vertCount; i++) {
		m_vertexData[i].position = { m_radius * cos(theta), m_radius * sin(theta), 0.0f };
		m_vertexData[i].uv = { 0.5f * (cos(theta) + 1.0f), 0.5f * (sin(theta) + 1.0f) };

		theta -= offset;
	}

	size_t p = 1;
	for (size_t i = 0; i < m_polyCount * 3; i += 3) {
		m_indices[i] = 0;
		m_indices[i + 1] = p;
		m_indices[i + 2] = (p + 1) >= m_vertCount ? 1 : (p + 1);
		p += 1;
	}

	std::unique_ptr <DirectX::XMFLOAT3[]> faceNorm(new DirectX::XMFLOAT3[m_polyCount]);
	for (size_t i = 0; i < m_polyCount; i++) {
		faceNorm[i] = { 0.0f, 0.0f, -1.0f };
	}

	SetShading(faceNorm.get());
	SetBuffers(_pContext);
}

//
// ---------- class Cuboid
//

DX::Cuboid::Cuboid(
	float _width, float _height, float _depth, 
	ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext,
	DX::SHADING _shading) : Mesh(8U, 12U, _shading) {

	m_width = _width;
	m_height = _height;
	m_depth = _depth;

	CreateBuffers(_pDevice);
	InitVertices(_pContext);
}

void DX::Cuboid::InitVertices(ID3D11DeviceContext* _pContext) {
	m_vertexData = std::unique_ptr <detail::MESH_VERTEX_DATA[]> (new detail::MESH_VERTEX_DATA[m_vertCount]);
	m_indices = std::unique_ptr <uint32_t[]> (new uint32_t[m_polyCount * 3]);

	int index = 0;
	float x = -m_width / 2, y = m_height / 2, z = -m_depth / 2;
	
	// set position and indices
	for (int i = 0; i < 2; i += 1, z += m_depth) {
		for (int j = 0; j < 2; j += 1, y -= m_height) {
			for (int k = 0; k < 2; k += 1, x += m_width) {
				m_vertexData[index].position = DirectX::XMFLOAT3 { x, y, z };
				m_vertexData[index].uv = { 0.5f * (1.0f + x / m_width), 0.5f * (1.0f + y / m_height) };
				index += 1;
			}
			x = -m_width / 2;
		}
		y = m_height / 2;
	}

	std::vector<uint32_t> t_indices = {
		// top
		0, 1, 2,
		1, 3, 2,
		// bottom
		4, 6, 5,
		5, 6, 7,
		// left
		0, 2, 4,
		2, 6, 4,
		// right
		3, 1, 5,
		3, 5, 7,
		// front
		2, 3, 6,
		3, 7, 6,
		// back
		1, 0, 5,
		0, 4, 5
	};
	for (size_t i = 0; i < t_indices.size(); i++) {
		m_indices[i] = t_indices[i];
	}

	// normals
	std::unique_ptr <DirectX::XMFLOAT3[]> faceNorm(new DirectX::XMFLOAT3[m_polyCount]);
	for (size_t i = 0; i < m_polyCount * 3; i += 3) {
		DirectX::XMFLOAT3 a, b;

		a = DX::Math::XMFloat3Subtract(m_vertexData[m_indices[i + 1]].position, m_vertexData[m_indices[i]].position);
		b = DX::Math::XMFloat3Subtract(m_vertexData[m_indices[i + 2]].position, m_vertexData[m_indices[i]].position);

		DirectX::XMStoreFloat3(
			&faceNorm[i / 3],
			DirectX::XMVector3Normalize(DirectX::XMVector3Cross(
				DirectX::XMLoadFloat3(&a),
				DirectX::XMLoadFloat3(&b)
			))
		);
	}

	SetShading(faceNorm.get());

	// manually set UV coordinates for now, replace this to use LSCM or ABF++ later
	if (m_shadingMode == DX::SHADING::FLAT) {
		m_vertexData[12].uv = { 0.0f, 0.0f }; m_vertexData[13].uv = { 1.0f, 0.0f }; m_vertexData[14].uv = { 0.0f, 1.0f };
		m_vertexData[15].uv = { 1.0f, 0.0f }; m_vertexData[16].uv = { 1.0f, 1.0f }; m_vertexData[17].uv = { 0.0f, 1.0f };
		m_vertexData[18].uv = { 0.0f, 0.0f }; m_vertexData[19].uv = { 1.0f, 0.0f }; m_vertexData[20].uv = { 1.0f, 1.0f };
		m_vertexData[21].uv = { 0.0f, 0.0f }; m_vertexData[22].uv = { 1.0f, 1.0f }; m_vertexData[23].uv = { 0.0f, 1.0f };
		m_vertexData[24].uv = { 0.0f, 0.0f }; m_vertexData[25].uv = { 1.0f, 0.0f }; m_vertexData[26].uv = { 0.0f, 1.0f };
		m_vertexData[27].uv = { 1.0f, 0.0f }; m_vertexData[28].uv = { 1.0f, 1.0f }; m_vertexData[29].uv = { 0.0f, 1.0f };
		m_vertexData[30].uv = { 1.0f, 1.0f }; m_vertexData[31].uv = { 0.0f, 1.0f }; m_vertexData[32].uv = { 1.0f, 0.0f };
		m_vertexData[33].uv = { 0.0f, 1.0f }; m_vertexData[34].uv = { 0.0f, 0.0f }; m_vertexData[35].uv = { 1.0f, 0.0f };
	}

	SetBuffers(_pContext);
}

//
// ---------- class Sphere
//

DX::Sphere::Sphere(
	float _radius, uint32_t _resX, uint32_t _resY,
	ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext,
	DX::SHADING _shading) : Mesh(
		static_cast <size_t> (_resX) * (std::max(_resY, 2U) - 1U) + 2U,
		static_cast <size_t> (_resX) * (static_cast <size_t> ((std::max(_resY, 2U) - 2U)) * 2U + 2U),
		_shading) {
	
	m_radius = _radius;
	m_resX = _resX;
	m_resY = std::max(_resY, 2U);

	if (m_resX < 3 || m_resY < 3) return;

	CreateBuffers(_pDevice);
	InitVertices(_pContext);
}

void DX::Sphere::InitVertices(ID3D11DeviceContext* _pContext) {
	m_vertexData = std::unique_ptr <detail::MESH_VERTEX_DATA[]> (new detail::MESH_VERTEX_DATA[m_vertCount]);
	m_indices = std::unique_ptr <uint32_t[]> (new uint32_t[m_polyCount * 3]);

	float theta = DX::Math::PI, phi;
	float incrY = -DX::Math::PI / m_resY, incrX = -DX::Math::PIx2 / m_resX;

	m_vertexData[0].position = { 0.0f, 0.0f, -m_radius };
	m_vertexData[0].normal = { 0.0f, 0.0f, -1.0f };
	theta += incrY;

	// generate vertices

	int index = 1;
	for (size_t i = 0; i < m_resY - 1U; i++) {
		phi = DX::Math::PIx2;
		for (size_t j = 0; j < m_resX; j++) {
			m_vertexData[index].position = {
				m_radius * sin(theta) * cos(phi),
				m_radius * sin(theta) * sin(phi),
				m_radius * cos(theta)
			};

			phi += incrX;
			index += 1;
		}
		theta += incrY;
	}

	m_vertexData[index].position = DirectX::XMFLOAT3 { 0.0f, 0.0f, m_radius };
	m_vertexData[index].normal = DirectX::XMFLOAT3 { 0.0f, 0.0f, 1.0f };

	// generate indices

	// north pole
	index = 0;
	for (size_t i = 0; i < m_resX; i++) {
		m_indices[index] = 0;
		m_indices[index + 1] = i + 1;
		m_indices[index + 2] = (i + 1) % m_resX + 1;
	
		index += 3;
	}

	// centeral polygons
	uint32_t vertex = m_resX + 1;
	for (size_t i = 0; i < m_resY - 2; i++) {
		for (size_t j = 0; j < m_resX; j++) {
			m_indices[index] = vertex - m_resX;
			m_indices[index + 1] = vertex;
			m_indices[index + 2] = ((j + 1) == m_resX) ? (vertex - m_resX + 1) : (vertex + 1);

			m_indices[index + 3] = m_indices[index + 2];
			m_indices[index + 4] = m_indices[index + 2] - m_resX;
			m_indices[index + 5] = m_indices[index];

			index += 6;
			vertex += 1;
		}
	}

	// south pole
	uint32_t fVertex = vertex;
	vertex = vertex - m_resX;
	for (size_t i = 0; i < m_resX; i++) {
		m_indices[index] = vertex;
		m_indices[index + 1] = fVertex;
		m_indices[index + 2] = (i + 1 == m_resX) ? (vertex - m_resX + 1) : (vertex + 1);
	
		index += 3;
		vertex += 1;
	}

	// face normals

	std::unique_ptr <DirectX::XMFLOAT3[]> faceNorm(new DirectX::XMFLOAT3[m_polyCount]);
	for (size_t i = 0; i < m_polyCount * 3; i += 3) {
		DirectX::XMFLOAT3 a, b;

		a = DX::Math::XMFloat3Subtract(m_vertexData[m_indices[i + 1]].position, m_vertexData[m_indices[i]].position);
		b = DX::Math::XMFloat3Subtract(m_vertexData[m_indices[i + 2]].position, m_vertexData[m_indices[i]].position);

		DirectX::XMStoreFloat3(
			&faceNorm[i / 3],
			DirectX::XMVector3Normalize(DirectX::XMVector3Cross(
				DirectX::XMLoadFloat3(&a),
				DirectX::XMLoadFloat3(&b)
			))
		);
	}

	for (size_t i = 0; i < m_vertCount; i++) {
		m_vertexData[i].uv = {
			0.5f + atan2(m_vertexData[i].position.y, m_vertexData[i].position.x) / DX::Math::PIx2,
			0.5f + asin(m_vertexData[i].position.z) / DX::Math::PI
		};
	}

	SetShading(faceNorm.get());
	SetBuffers(_pContext);
}

//
// ---------- class Plane
//

DX::Plane::Plane(
	float _width, float _length, uint32_t _resX, uint32_t _resY,
	ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext,
	DX::SHADING _shading
) : Mesh((_resX + 2U) * (_resY + 2U), 2U * (_resX + 1U) * (_resY + 1U), _shading) {
	m_width = _width;
	m_length = _length;
	m_resX = _resX;
	m_resY = _resY;

	CreateBuffers(_pDevice);
	InitVertices(_pContext);
}

void DX::Plane::InitVertices(ID3D11DeviceContext* _pContext) {
	m_vertexData = std::unique_ptr <detail::MESH_VERTEX_DATA[]>(new detail::MESH_VERTEX_DATA[m_vertCount]);
	m_indices = std::unique_ptr <uint32_t[]>(new uint32_t[m_polyCount * 3]);

	// vertex data

	float offsetX = m_width / (m_resX + 1);
	float offsetY = -m_length / (m_resY + 1);
	float y = m_length / 2, x;
	for (size_t i = 0; i < static_cast <size_t> (m_resY) + 2U; i++) {
		x = -m_width / 2;

		for (size_t j = 0; j < static_cast <size_t> (m_resX) + 2U; j++) {
			size_t index = i * (static_cast <size_t> (m_resX) + 2U) + j;

			m_vertexData[index].position = DirectX::XMFLOAT3 { x, y, 0.0f };
			m_vertexData[index].uv = DirectX::XMFLOAT2 { 2.0f * x / m_width, 2.0f * y / m_length };
			
			x += offsetX;
		}
		y += offsetY;
	}

	// face normals

	std::unique_ptr<DirectX::XMFLOAT3[]> faceNorm(new DirectX::XMFLOAT3[m_polyCount]);
	for (size_t i = 0; i < m_polyCount; i++) {
		faceNorm[i] = { 0.0f, 0.0f, -1.0f };
	}

	// indices

	uint32_t index = 0;
	for (size_t i = 0; i <= m_resY; i++) {
		for (size_t j = 0; j <= m_resX; j++) {
			size_t t_i = i * (static_cast <size_t> (m_resX) + 2U) + j;
			assert(static_cast <size_t> (index) + 5 < m_polyCount * 3);

			m_indices[index] = t_i;
			m_indices[index + 1] = t_i + 1;
			m_indices[index + 2] = t_i + m_resX + 2U;
			index += 3;

			m_indices[index] = t_i + 1;
			m_indices[index + 1] = t_i + m_resX + 3U;
			m_indices[index + 2] = t_i + m_resX + 2U;
			index += 3;
		}
	}

	SetShading(faceNorm.get());
	SetBuffers(_pContext);
}

//
// ---------- class CustomMesh
//

DX::CustomMesh::CustomMesh():Mesh(0, 0, DX::SHADING::FLAT) { }

HRESULT DX::CustomMesh::LoadFromFile(_In_ std::string _fName, _In_ ID3D11Device* _pDevice, _In_ ID3D11DeviceContext* _pContext, _Out_opt_ char* _log) {
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(_fName,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices
	);

	if (scene == nullptr) {
		if (_log) {
			const char* errLog = importer.GetErrorString();
			size_t errLen = 0;
			for (; *(errLog + errLen) != '\0'; errLen++);
			memcpy(_log, errLog, errLen * sizeof (char));
		}
		return E_FAIL;
	}

	if (scene->mNumMeshes < 1) {
		std::string errLog = "Scene does not contain any meshes";
		memcpy(_log, errLog.c_str(), errLog.size());
		return S_OK;
	}

	aiMesh* mesh = scene->mMeshes[0];
	m_vertCount = mesh->mNumVertices;
	m_polyCount = mesh->mNumFaces;

	CreateBuffers(_pDevice);

	if (m_vertCount < 3 || m_polyCount < 1) {
		if (_log) {
			std::string errLog = "No vertexData found";
			memcpy(_log, errLog.c_str(), errLog.size());
		}
		return S_OK;
	}

	m_vertexData = std::unique_ptr <detail::MESH_VERTEX_DATA[]>(new detail::MESH_VERTEX_DATA[m_vertCount]);
	m_indices = std::unique_ptr <uint32_t[]>(new uint32_t[m_polyCount * 3]);

	for (size_t i = 0; i < m_vertCount; i++) {
		m_vertexData[i].position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		m_vertexData[i].normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		m_vertexData[i].uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
	}

	aiFace* faces = mesh->mFaces;
	for (size_t i = 0; i < m_polyCount; i++) {
		m_indices[i * 3] = faces[i].mIndices[0];
		m_indices[i * 3 + 1] = faces[i].mIndices[1];
		m_indices[i * 3 + 2] = faces[i].mIndices[2];
	}

	SetBuffers(_pContext);

	return S_OK;
}

void DX::CustomMesh::InitVertices(ID3D11DeviceContext* _pContext) {  }