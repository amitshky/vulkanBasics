#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "buffer/vertexBuffer.h"


class Model
{
public:
	Model(const char* modelPath);

	inline std::vector<Vertex> GetVertices() const { return m_Vertices; }
	inline std::vector<uint32_t> GetIndices() const { return m_Indices; }

private:
	void LoadModel();

private:
	const char* m_ModelPath;

	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
};
