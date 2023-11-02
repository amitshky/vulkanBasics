#include "commandBuffer.h"

#include <stdexcept>
#include <array>

#include "utils/utils.h"


CommandBuffer::CommandBuffer(const int maxFramesInFlight, VkSurfaceKHR windowSurface, const Device* device)
	: m_MaxFramesInFlight{ maxFramesInFlight },
	  m_WindowSurface{ windowSurface },
	  m_Device{ device }
{
	CreateCommandPool();
	CreateCommandBuffers();
}

CommandBuffer::~CommandBuffer()
{
	// command buffers are automatically destroyed when their command pool is
	// destroyed
	vkDestroyCommandPool(m_Device->GetDevice(), m_CommandPool, nullptr);
}

void CommandBuffer::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = utils::FindQueueFamilies(m_Device->GetPhysicalDevice(), m_WindowSurface);

	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allows command
																				   // buffers to be
																				   // recorded
																				   // individually
	commandPoolCreateInfo.queueFamilyIndex =
		queueFamilyIndices.graphicsFamily.value(); // command pools can only allocate command buffers that
												   // are submitted by a single type of queue

	if (vkCreateCommandPool(m_Device->GetDevice(), &commandPoolCreateInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pools!");
}

void CommandBuffer::CreateCommandBuffers()
{
	m_CommandBuffers.resize(m_MaxFramesInFlight);

	// command buffer allocation
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_CommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // can be submitted to a queue for
																	   // execution, but cannot be called from
																	   // other command buffers
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

	if (vkAllocateCommandBuffers(m_Device->GetDevice(), &commandBufferAllocateInfo, m_CommandBuffers.data())
		!= VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers!");
}

void CommandBuffer::ResetCommandBuffer(int32_t index)
{
	vkResetCommandBuffer(m_CommandBuffers[index], 0);
}
