#include "model.h"

#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"


Model::Model(const char* modelPath)
	: m_ModelPath{modelPath}
{
	LoadModel();
}

void Model::LoadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, m_ModelPath))
		throw std::runtime_error(warn + err);

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const tinyobj::shape_t& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				// 0 coordinate in an obj file means bottom of the image
				// but here 0 means top
				// so we flip the coordinate
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			// check if the vertex is already in the map
			if (uniqueVertices.count(vertex) == 0)
				uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size()); // get the index of the vertex

			m_Vertices.push_back(vertex);
			m_Indices.push_back(uniqueVertices[vertex]);
		}
	}
}
