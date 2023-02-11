#include "bufferUtils.h"

#include <stdexcept>

#include "utils.h"
#include "utils/commandBufferUtils.h"


namespace utils
{
namespace buff
{

void CreateBuffer(VkDevice deviceVk, VkPhysicalDevice physicalDevice, 
	VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
	VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size        = size;
	bufferCreateInfo.usage       = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffers can be owned by a specific queue family or be shared between multiple at the same time

	if (vkCreateBuffer(deviceVk, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create vertex buffer!");

	// assign memory to the buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(deviceVk, buffer, &memRequirements);

	// memory allocation
	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize  = memRequirements.size;
	memAllocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	// we are not supposed to call vkAllocateMemory() for every individual buffer, because we have a limited maxMemoryAllocationCount
	// instead we can allocate a large memory and use offset to split the memory
	if (vkAllocateMemory(deviceVk, &memAllocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	
	vkBindBufferMemory(deviceVk, buffer, bufferMemory, 0);
}

void CopyBuffer(VkDevice deviceVk, VkQueue graphicsQueue, VkCommandPool commandPool, 
	VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer cmdBuff = cmd::BeginSingleTimeCommands(deviceVk, commandPool);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size      = size;
	// transfer the contents of the buffers
	vkCmdCopyBuffer(cmdBuff, srcBuffer, dstBuffer, 1, &copyRegion);
	
	cmd::EndSingleTimeCommands(deviceVk, graphicsQueue, commandPool, cmdBuff);
}

void CopyBufferToImage(VkDevice deviceVk, VkQueue graphicsQueue, VkCommandPool commandPool, 
	VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer cmdBuff = cmd::BeginSingleTimeCommands(deviceVk, commandPool);

	// specify which part of the buffer is going to be copied to which part of the image
	VkBufferImageCopy region{};
	region.bufferOffset      = 0;
	region.bufferImageHeight = 0;
	region.bufferRowLength   = 0;
	
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	// part of the image to copy to	
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(cmdBuff, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	
	cmd::EndSingleTimeCommands(deviceVk, graphicsQueue, commandPool, cmdBuff);
}

} // namespace buff
} // namespace utils