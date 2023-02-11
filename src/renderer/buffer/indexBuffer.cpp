#include "indexBuffer.h"

#include <stdexcept>

#include "renderer/swapchain.h"
#include "utils/bufferUtils.h"


IndexBuffer::IndexBuffer(const Device* device, const CommandBuffer* commandBuffers, const std::vector<uint16_t>& indices)
	: m_Device{device},
	  m_CommandBuffers{commandBuffers},
	  m_Indices{indices}
{
	CreateIndexBuffer();
}

IndexBuffer::~IndexBuffer()
{
	vkDestroyBuffer(m_Device->GetDevice(), m_IndexBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_BufferMemory, nullptr);
}

void IndexBuffer::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	utils::buff::CreateBuffer(m_Device->GetDevice(), m_Device->GetPhysicalDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // source memory during transfer
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory); // staging buffer (in the CPU); temporary buffer

	// copy the vertex data to the buffer
	void* data;
	vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data); // mapping the buffer memory into CPU accessible memory
	memcpy(data, m_Indices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

	utils::buff::CreateBuffer(m_Device->GetDevice(), m_Device->GetPhysicalDevice(), bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, // destination memory during transfer
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_BufferMemory); // the actual buffer (located in the device memory)

	utils::buff::CopyBuffer(m_Device->GetDevice(), m_Device->GetGraphicsQueue(), m_CommandBuffers->GetCommandPool(), 
		stagingBuffer, m_IndexBuffer, bufferSize);

	vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);
}