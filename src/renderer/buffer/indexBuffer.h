#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "renderer/device.h"
#include "renderer/buffer/commandBuffer.h"


class IndexBuffer 
{
public:
	IndexBuffer(const Device* device, const CommandBuffer* commandBuffers, const std::vector<uint32_t>& indices);
	~IndexBuffer();

	inline VkBuffer GetIndexBuffer() const { return m_IndexBuffer; }

private:
	void CreateIndexBuffer();

private:
	const Device* m_Device;
	const CommandBuffer* m_CommandBuffers; 

	std::vector<uint32_t> m_Indices;

	VkBuffer m_IndexBuffer;
	VkDeviceMemory m_BufferMemory;
};
