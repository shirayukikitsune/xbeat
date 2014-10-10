#include "OBJModel.h"
#include "../D3DRenderer.h"

#include <fstream>
#include <sstream>
#include <algorithm>

using namespace Renderer;

OBJModel::OBJModel()
{
}


OBJModel::~OBJModel()
{
}

bool OBJModel::InitializeBuffers(std::shared_ptr<Renderer::D3DRenderer> d3d)
{
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	DirectX::XMVECTOR normal, uv;

	std::vector<Shaders::Light::VertexInput> vertices;
	std::vector<UINT> indices;

	vertices.resize(m_vertexCount, Shaders::Light::VertexInput{ DirectX::XMFLOAT3(NAN, NAN, NAN), DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT2(0, 0) });

	for (auto &face : m_faces) {
		for (size_t i = 0; i < face->vertices.size(); i++) {
			uint32_t index = face->vertices[i] - 1;
			normal = DirectX::XMLoadFloat3(&vertices[index].normal);
			if (i < face->normals.size() && face->normals[i] > 0U) {
				normal = DirectX::XMVector3Normalize(DirectX::XMVectorAdd(normal, m_normals[face->normals[i] - 1]));
			}

			if (!DirectX::XMVector3IsNaN(DirectX::XMLoadFloat3(&vertices[index].position))) {
				DirectX::XMStoreFloat3(&vertices[index].normal, normal);
			}
			else {
				uv = DirectX::XMLoadFloat2(&vertices[index].uv);
				if (i < face->uvs.size() && face->uvs[i] > 0U) uv = m_uvs[face->uvs[i] - 1];
				DirectX::XMStoreFloat3(&vertices[index].position, m_vertices[index]);
				DirectX::XMStoreFloat3(&vertices[index].normal, normal);
				DirectX::XMStoreFloat2(&vertices[index].uv, uv);
			}
			indices.emplace_back(index);
		}
	}

	//m_mesh.reset(new btTriangleIndexVertexArray((int)indices.size() / 3, (int*)indices.data(), sizeof(UINT), (int)vertices.size(), &vertices[0].position.x, sizeof(Shaders::Light::VertexInput)));
	//m_shape.reset(new btBvhTriangleMeshShape(m_mesh.get(), true));
	m_shape.reset(new btStaticPlaneShape(btVector3(0, 1, 0), 0));
	m_body.reset(new btRigidBody(0, new btDefaultMotionState(), m_shape.get()));
	m_physics->addRigidBody(m_body);

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = (UINT)(sizeof(Shaders::Light::VertexInput) * vertices.size());
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = vertices.data();
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = d3d->GetDevice()->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
		return false;

	m_indexCount = (int)indices.size();

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(UINT) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = indices.data();
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = d3d->GetDevice()->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
		return false;

	return true;
}

bool OBJModel::LoadTexture(ID3D11Device *device)
{
	m_texture.reset(new Texture);
	if (!m_texture)
		return false;

	if (!m_texture->Initialize(device, L"./Data/Textures/grass.jpg"))
		return false;

	return true;
}

void OBJModel::ShutdownBuffers()
{
	if (m_indexBuffer) {
		m_indexBuffer->Release();
		m_indexBuffer = nullptr;
	}

	if (m_vertexBuffer) {
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}
}

void OBJModel::ReleaseTexture()
{
	m_texture.reset();
}

void OBJModel::Render(ID3D11DeviceContext* context, std::shared_ptr<ViewFrustum> frustum)
{
	unsigned int stride;
	unsigned int offset;
	DirectX::XMFLOAT3 cameraPosition;

	// Set vertex buffer stride and offset.
	stride = sizeof(Shaders::Light::VertexInput);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tex = m_texture->GetTexture();
	context->PSSetShaderResources(0, 1, &tex);

	m_shader->PrepareRender(context);

	m_shader->Render(context, m_indexCount, 0);
}

// Adapted from http://www.cplusplus.com/faq/sequences/strings/split/
enum struct splitflags
{
	none,
	no_empties,
	trim
};

template <typename T>
bool has_flag(T flags, T flag)
{
	return (T)((uint32_t)flags & (uint32_t)flag) == flag;
}

template <typename Container>
Container& split(
	Container&                                 result,
	const typename Container::value_type&      s,
	const typename Container::value_type&      delimiter,
	splitflags                                 flags = splitflags::none)
{
	result.clear();
	Container::value_type::size_type now = 0, last = 0;
	do
	{
		now = s.find_first_of(delimiter, last);
		typename Container::value_type field = s.substr(last, now - last);
		if (has_flag<splitflags>(flags, splitflags::no_empties) && field.empty()) continue;
		if (has_flag<splitflags>(flags, splitflags::trim)) {
			Container::value_type::size_type begin = field.find_first_not_of(" \t\f\v\n\r");
			if (begin != Container::value_type::npos) field.erase(0, begin);
			Container::value_type::size_type end = field.find_last_not_of(" \t\f\v\n\r");
			if (end != Container::value_type::npos) field.erase(end + 1);
		}
		result.push_back(field);
		now = s.find_first_not_of(delimiter, now);
		last = now;
	} while (now != Container::value_type::npos);
	return result;
}

bool OBJModel::LoadModel(const std::wstring &filename)
{
	std::ifstream fin;
	fin.open(filename);
	if (fin.fail())
		return false;

	std::string line;
	while (std::getline(fin, line)) {
		if (line.empty() || line[0] == '#')
			continue; // Empty lines or lines starting with # are comments

		std::vector<std::string> fields, faces;
		split<std::vector<std::string>>(fields, line, " \t\f\v\n\r", splitflags::trim);
		if (fields.empty()) continue;
		if (fields[0].compare("v") == 0) {
			// Vertex definition
			float W = 0.0f;
			if (fields.size() > 4) W = std::stof(fields[4]);
			m_vertices.emplace_back(DirectX::XMVectorSet(std::stof(fields[1]), std::stof(fields[2]), std::stof(fields[3]), W));
		}
		else if (fields[0].compare("vt") == 0) {
			// Vertex UV definition
			float Z = 0.0f;
			if (fields.size() > 3) Z = std::stof(fields[3]);
			m_uvs.emplace_back(DirectX::XMVectorSet(std::stof(fields[1]), std::stof(fields[2]), Z, 0.0f));
		}
		else if (fields[0].compare("vn") == 0) {
			// Vertex normal definition
			m_normals.emplace_back(DirectX::XMVectorSet(std::stof(fields[1]), std::stof(fields[2]), std::stof(fields[3]), 0.0f));
		}
		else if (fields[0].compare("f") == 0) {
			// Faces definitions
			Face *face = new Face;
			for (uint32_t i = 1; i < fields.size(); i++) {
				split<std::vector<std::string>>(faces, fields[i], "/", splitflags::trim);
				face->vertices.emplace_back(std::stoi(faces[0]));
				if (faces.size() > 1) face->uvs.emplace_back(faces[1].empty() ? -1 : std::stoi(faces[1]));
				else face->uvs.emplace_back(0);
				if (faces.size() > 2) face->normals.emplace_back(std::stoi(faces[2]));
				else face->normals.emplace_back(0);

				if (face->vertices.back() < 0) face->vertices.back() += (int)face->vertices.size();
				if (face->normals.back() < 0) face->normals.back() += (int)face->normals.size();
				if (face->uvs.back() < 0) face->uvs.back() += (int)face->uvs.size();

				m_vertexCount = std::max(m_vertexCount, face->vertices.back());
			}
			m_faces.emplace_back(face);
		}
	}

	fin.close();

	return true;
}

void OBJModel::ReleaseModel()
{
	m_vertices.clear();
	m_uvs.clear();
	m_normals.clear();
	m_faces.clear();
}
