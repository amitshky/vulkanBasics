#pragma once

#include <array>

#include <vulkan/vulkan.h>
#include "glm/glm.hpp"


// TODO: move this to vertex buffer class maybe
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


class Pipeline
{
public:
	Pipeline(VkDevice device, VkRenderPass renderPass);
	~Pipeline();

	inline VkPipeline GetPipeline() const { return m_Pipeline; }
	inline VkPipelineLayout GetLayout() const { return m_PipelineLayout; }

	inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

private:
	void CreateGraphicsPipeline();
	void CreateDescriptorSetLayout(); // TODO: move this to buffers (vertex or uniform)

private:
	VkDevice m_Device;
	VkRenderPass m_RenderPass;

	VkDescriptorSetLayout m_DescriptorSetLayout;

	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_Pipeline;
};