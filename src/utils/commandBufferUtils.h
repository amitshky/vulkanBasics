#pragma once

#include <vulkan/vulkan.h>


namespace utils {
namespace cmd {

VkCommandBuffer BeginSingleTimeCommands(VkDevice deviceVk, VkCommandPool commandPool);
void EndSingleTimeCommands(VkDevice deviceVk,
	VkQueue graphicsQueue,
	VkCommandPool commandPool,
	VkCommandBuffer cmdBuff);

} // namespace cmd
} // namespace utils