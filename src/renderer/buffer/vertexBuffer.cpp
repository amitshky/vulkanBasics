#include "vertexBuffer.h"

#include <stdexcept>

#include "renderer/swapchain.h"


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
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // source memory during transfer
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory); // staging buffer (in the CPU); temporary buffer

	// copy the vertex data to the buffer
	void* data;
	vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data); // mapping the buffer memory into CPU accessible memory
	memcpy(data, m_Vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // destination memory during transfer
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_BufferMemory); // the actual buffer (located in the device memory)

	CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

	vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);

	// the driver may not immediately copy the data into the buffer memory
	// so the writes to the buffer may not be visible yet
	// this is why we set VK_MEMORY_PROPERTY_HOST_COHERENT_BIT in the memory type
	// so that the mapped memory always matches the contents of the allocated memory
}

void VertexBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size        = size;
	bufferCreateInfo.usage       = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffers can be owned by a specific queue family or be shared between multiple at the same time

	if (vkCreateBuffer(m_Device->GetDevice(), &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create vertex buffer!");

	// assign memory to the buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_Device->GetDevice(), buffer, &memRequirements);

	// memory allocation
	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize  = memRequirements.size;
	memAllocInfo.memoryTypeIndex = Swapchain::FindMemoryType(m_Device->GetPhysicalDevice(), memRequirements.memoryTypeBits, properties);

	// we are not supposed to call vkAllocateMemory() for every individual buffer, because we have a limited maxMemoryAllocationCount
	// instead we can allocate a large memory and use offset to split the memory
	if (vkAllocateMemory(m_Device->GetDevice(), &memAllocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	
	vkBindBufferMemory(m_Device->GetDevice(), buffer, bufferMemory, 0);
}

void VertexBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer cmdBuff = m_CommandBuffers->BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size      = size;
	// transfer the contents of the buffers
	vkCmdCopyBuffer(cmdBuff, srcBuffer, dstBuffer, 1, &copyRegion);
	
	m_CommandBuffers->EndSingleTimeCommands(cmdBuff);
}