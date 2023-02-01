#include "commandBuffers.h"

#include <stdexcept>
#include <array>


CommandBuffers::CommandBuffers(const VulkanConfig* config, VkSurfaceKHR windowSurface, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue)
	: m_Config{config},
	  m_WindowSurface{windowSurface},
	  m_PhysicalDevice{physicalDevice},
	  m_Device{device},
	  m_GraphicsQueue{graphicsQueue}
{
	CreateCommandPool();
	CreateCommandBuffers();
}

CommandBuffers::~CommandBuffers()
{
	// command buffers are automatically destroyed when their command pool is destroyed
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
}

void CommandBuffers::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = Device::FindQueueFamilies(m_PhysicalDevice, m_WindowSurface);

	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allows command buffers to be recorded individually
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();       // command pools can only allocate command buffers that are submitted by a single type of queue

	if (vkCreateCommandPool(m_Device, &commandPoolCreateInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pools!");
}

void CommandBuffers::CreateCommandBuffers()
{
	m_CommandBuffers.resize(m_Config->MAX_FRAMES_IN_FLIGHT);

	// command buffer allocation
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool        = m_CommandPool;
	commandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // can be submitted to a queue for execution, but cannot be called from other command buffers
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

	if (vkAllocateCommandBuffers(m_Device, &commandBufferAllocateInfo, m_CommandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers!");
}

void CommandBuffers::ResetCommandBuffer(int32_t index)
{
	vkResetCommandBuffer(m_CommandBuffers[index], 0);
}

VkCommandBuffer CommandBuffers::BeginSingleTimeCommands()
{
	// transfer operations are also executed using command buffers
	// so we allocate a temporary command buffer
	VkCommandBufferAllocateInfo cmdBuffAllocInfo{};
	cmdBuffAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBuffAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBuffAllocInfo.commandPool        = m_CommandPool;
	cmdBuffAllocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuff;
	vkAllocateCommandBuffers(m_Device, &cmdBuffAllocInfo, &cmdBuff);

	// immediately start recording the command buffer
	VkCommandBufferBeginInfo cmdBuffBegin{};
	cmdBuffBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBuffBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmdBuff, &cmdBuffBegin);

	return cmdBuff;
}

void CommandBuffers::EndSingleTimeCommands(VkCommandBuffer cmdBuff)
{
	vkEndCommandBuffer(cmdBuff);

	// submit the queue
	VkSubmitInfo submitInfo{};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &cmdBuff;

	// we dont necessarily need a transfer queue, graphics queue can handle it
	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	// we can use a fence to wait; this can help to schedule multiple transfers simultaneously
	vkQueueWaitIdle(m_GraphicsQueue);

	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &cmdBuff);
}