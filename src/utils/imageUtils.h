#pragma once

#include <vulkan/vulkan.h>


namespace utils
{
namespace img
{

void CreateImage(VkDevice deviceVk, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, 
	VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
	VkImage& image, VkDeviceMemory& imageMemory);

VkImageView CreateImageView(VkDevice deviceVk, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

} // namespace img
} // namespace utils