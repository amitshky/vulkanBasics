#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "core/vulkanConfig.h"
#include "device.h"


class CommandBuffers
{
public:
	CommandBuffers(const VulkanConfig* config, VkSurfaceKHR windowSurface, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue);
	~CommandBuffers();

	void ResetCommandBuffer(int32_t index);

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer cmdBuff);

	inline VkCommandBuffer& GetCommandBufferAtIndex(int32_t index) { return m_CommandBuffers[index]; }
	inline VkCommandBuffer GetCommandBufferAtIndex(int32_t index) const { return m_CommandBuffers[index]; }

private:
	void CreateCommandPool();
	void CreateCommandBuffers();

private:
	const VulkanConfig* m_Config;
	VkSurfaceKHR m_WindowSurface;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
	VkQueue m_GraphicsQueue;

	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
};