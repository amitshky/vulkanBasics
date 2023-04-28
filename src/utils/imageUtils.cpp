#include "imageUtils.h"

#include <stdexcept>

#include "utils.h"


namespace utils
{
namespace img
{

void CreateImage(VkDevice deviceVk, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, 
	VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imgCreateInfo{};
	imgCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
	imgCreateInfo.extent.width  = width;
	imgCreateInfo.extent.height = height;
	imgCreateInfo.extent.depth  = 1;
	imgCreateInfo.mipLevels     = 1;
	imgCreateInfo.arrayLayers   = 1;
	imgCreateInfo.format        = format;
	imgCreateInfo.tiling        = tiling; // Texels are laid out in an implementation defined order for optimal access
	imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // not needed because we copy the texel data from the buffer
	imgCreateInfo.usage         = usage; // `VK_IMAGE_USAGE_SAMPLED_BIT` because we also want the image to be accessed from the shader
	imgCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	imgCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT; // for multisampling
	imgCreateInfo.flags         = 0; // for sparse images

	if (vkCreateImage(deviceVk, &imgCreateInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image object!");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(deviceVk, image, &memRequirements);

	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = utils::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(deviceVk, &memAllocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate image memory!");

	vkBindImageMemory(deviceVk, image, imageMemory, 0);
}

VkImageView CreateImageView(VkDevice deviceVk, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo imgViewCreateInfo{};
	imgViewCreateInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imgViewCreateInfo.image    = image;
	imgViewCreateInfo.format   = format;
	imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imgViewCreateInfo.subresourceRange.aspectMask     = aspectFlags;
	imgViewCreateInfo.subresourceRange.baseMipLevel   = 0;
	imgViewCreateInfo.subresourceRange.levelCount     = 1;
	imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imgViewCreateInfo.subresourceRange.layerCount     = 1;

	VkImageView imageView;
	if (vkCreateImageView(deviceVk, &imgViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image view!");

	return imageView;
}

} // namespace img
} // namespace utils
