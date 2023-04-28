#include "imageUtils.h"

#include <stdexcept>

#include "utils.h"
#include "commandBufferUtils.h"


namespace utils
{
namespace img
{

void CreateImage(VkDevice deviceVk, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imgCreateInfo{};
	imgCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
	imgCreateInfo.extent.width  = width;
	imgCreateInfo.extent.height = height;
	imgCreateInfo.extent.depth  = 1;
	imgCreateInfo.mipLevels     = mipLevels;
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

VkImageView CreateImageView(VkDevice deviceVk, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo imgViewCreateInfo{};
	imgViewCreateInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imgViewCreateInfo.image    = image;
	imgViewCreateInfo.format   = format;
	imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imgViewCreateInfo.subresourceRange.aspectMask     = aspectFlags;
	imgViewCreateInfo.subresourceRange.baseMipLevel   = 0;
	imgViewCreateInfo.subresourceRange.levelCount     = mipLevels;
	imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imgViewCreateInfo.subresourceRange.layerCount     = 1;

	VkImageView imageView;
	if (vkCreateImageView(deviceVk, &imgViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image view!");

	return imageView;
}

void GenerateMipmaps(VkDevice deviceVk, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mipLevels)
{
	// TODO: load mipmaps from a file instead of generating them

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
	// check image format's support for linear filter
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		throw std::runtime_error("Texture image format does not support linear blitting!");

	VkCommandBuffer cmdBuff = cmd::BeginSingleTimeCommands(deviceVk, commandPool);

	VkImageMemoryBarrier imgBarrier{};
	imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imgBarrier.image = image;
	imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgBarrier.subresourceRange.baseArrayLayer = 0;
	imgBarrier.subresourceRange.layerCount = 1;
	imgBarrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; ++i)
	{
		// trasition the `i - 1` mip level to `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`
		imgBarrier.subresourceRange.baseMipLevel = i - 1;
		imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier);

		// specify the region to be used in blit operation
		// the src mip level is `i - 1`
		// the dst mip level is `i`
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cmdBuff, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		// trasition the `i - 1` mip level to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
		// all the sampling operations will wait on this transition to finish
		imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier);

		// one of the dimensions will reach 1 before the other, so keep it 1 when it does (because the image is not a square)
		if (mipWidth > 1)
			mipWidth /= 2;

		if (mipHeight > 1)
			mipHeight /= 2;
	}

	// this barrier transitions the last mip level from `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
	// the loop doesnt handle this
	imgBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
	imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier);

	cmd::EndSingleTimeCommands(deviceVk, graphicsQueue, commandPool, cmdBuff);
}

} // namespace img
} // namespace utils
