#include "vertexBuffer.h"

#include <stdexcept>

#include "renderer/swapchain.h"
#include "utils/bufferUtils.h"


VertexBuffer::VertexBuffer(const Device* device, const CommandBuffer* commandBuffers, const std::vector<Vertex>& vertices)
	: m_Device{device},
	  m_CommandBuffers{commandBuffers},
	  m_Vertices{vertices}
{
	CreateVertexBuffer();
}

VertexBuffer::~VertexBuffer()
{
	vkDestroyBuffer(m_Device->GetDevice(), m_VertexBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_BufferMemory, nullptr);
}

void VertexBuffer::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

	// we create one buffer accessible by the CPU and another one in the device's local memory
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	utils::buff::CreateBuffer(m_Device->GetDevice(), m_Device->GetPhysicalDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // source memory during transfer
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory); // staging buffer (in the CPU); temporary buffer

	// copy the vertex data to the buffer
	void* data;
	vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data); // mapping the buffer memory into CPU accessible memory
	memcpy(data, m_Vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

	utils::buff::CreateBuffer(m_Device->GetDevice(), m_Device->GetPhysicalDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // destination memory during transfer
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_BufferMemory); // the actual buffer (located in the device memory)

	utils::buff::CopyBuffer(m_Device->GetDevice(), m_Device->GetGraphicsQueue(), m_CommandBuffers->GetCommandPool(), 
		stagingBuffer, m_VertexBuffer, bufferSize);

	vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);

	// the driver may not immediately copy the data into the buffer memory
	// so the writes to the buffer may not be visible yet
	// this is why we set VK_MEMORY_PROPERTY_HOST_COHERENT_BIT in the memory type
	// so that the mapped memory always matches the contents of the allocated memory
}