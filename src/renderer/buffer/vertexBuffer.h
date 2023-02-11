#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "renderer/device.h"
#include "renderer/buffer/commandBuffer.h"


struct Vertex 
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding   = 0; // specifies the index of the binding in the array of bindings
		bindingDescription.stride    = sizeof(Vertex); // specifies the number of bytes from one entry to the next
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // move to the next data entry after each vertex

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
	{
		// for position
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding  = 0; // index of the per-vertex data
		attributeDescriptions[0].location = 0; // references the location directive of the input in the vertex shader.
		attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT; // type of data; position has 2 floats
		attributeDescriptions[0].offset   = offsetof(Vertex, pos); // number of bytes from the begining of the per-vertex data
		// for color
		attributeDescriptions[1].binding  = 0; // index of the per-vertex data
		attributeDescriptions[1].location = 1; // references the location directive of the input in the vertex shader.
		attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT; // type of data; color has 3 floats
		attributeDescriptions[1].offset   = offsetof(Vertex, color); // number of bytes from the begining of the per-vertex data
		// for texture coordinates
		attributeDescriptions[2].binding  = 0; // index of the per-vertex data
		attributeDescriptions[2].location = 2; // references the location directive of the input in the vertex shader.
		attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT; // type of data; color has 3 floats
		attributeDescriptions[2].offset   = offsetof(Vertex, texCoord); // number of bytes from the begining of the per-vertex data
		return attributeDescriptions;
	}
};


class VertexBuffer 
{
public:
	VertexBuffer(const Device* device, const CommandBuffer* commandBuffers, const std::vector<Vertex>& vertices);
	~VertexBuffer();

	inline VkBuffer GetVertexBuffer() const { return m_VertexBuffer; }

private:
	void CreateVertexBuffer();

private:
	const Device* m_Device;
	const CommandBuffer* m_CommandBuffers; 
	
	std::vector<Vertex> m_Vertices;

	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_BufferMemory;
};