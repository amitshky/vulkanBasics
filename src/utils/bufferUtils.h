#pragma once

#include <vulkan/vulkan.h>


namespace utils {
namespace buff {

void CreateBuffer(VkDevice deviceVk,
	VkPhysicalDevice physicalDevice,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory);

void CopyBuffer(VkDevice deviceVk,
	VkQueue graphicsQueue,
	VkCommandPool commandPool,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize size);

void CopyBufferToImage(VkDevice deviceVk,
	VkQueue graphicsQueue,
	VkCommandPool commandPool,
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height);

} // namespace buff
} // namespace utils