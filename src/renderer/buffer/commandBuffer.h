#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "core/vulkanConfig.h"
#include "renderer/device.h"


class CommandBuffer
{
public:
	CommandBuffer(const int maxFramesInFlight, VkSurfaceKHR windowSurface, const Device* device);
	~CommandBuffer();

	void ResetCommandBuffer(int32_t index);

	inline VkCommandPool GetCommandPool() const { return m_CommandPool; }
	inline VkCommandBuffer& GetCommandBufferAtIndex(const uint32_t index) { return m_CommandBuffers[index]; }
	inline VkCommandBuffer GetCommandBufferAtIndex(const uint32_t index) const { return m_CommandBuffers[index]; }

private:
	void CreateCommandPool();
	void CreateCommandBuffers();

private:
	const int m_MaxFramesInFlight;
	VkSurfaceKHR m_WindowSurface;
	const Device* m_Device;

	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
};