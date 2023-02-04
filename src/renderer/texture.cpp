#include "texture.h"

#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#include "renderer/swapchain.h"


Texture::Texture(const Device* device, const CommandBuffer* commandBuffers)
	: m_Device{device},
	  m_CommandBuffers{commandBuffers}
{
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
}

Texture::~Texture()
{
	vkDestroySampler(m_Device->GetDevice(), m_TextureSampler, nullptr);
	vkDestroyImageView(m_Device->GetDevice(), m_TextureImageView, nullptr);
	vkDestroyImage(m_Device->GetDevice(), m_TextureImage, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_TextureImageMemory, nullptr);
}

void Texture::CreateTextureImage()
{
	int width = 0;
	int height = 0;
	int channels = 0;

	//                                                                               // force alpha (even if there isnt one)
	auto imgData = stbi_load("assets/textures/texture.jpg", &width, &height, &channels, STBI_rgb_alpha);
	VkDeviceSize imgSize = width * height * 4;

	if (!imgData)
		throw std::runtime_error("Failed to load texture image!");

	// create a staging buffer
	// we can use a staging image object but we are using VkBuffer

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(imgSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, imgSize, 0, &data);
	memcpy(data, imgData, static_cast<size_t>(imgSize));
	vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

	stbi_image_free(imgData);

	Swapchain::CreateImage(m_Device->GetDevice(), m_Device->GetPhysicalDevice(), static_cast<uint32_t>(width), static_cast<uint32_t>(height), 
		VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);

	// copy staging buffer to the texture image
	// transfer the image layout to DST_OPTIMAl
	TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

	// to start sampling from the texture image in the shader
	TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);
}

void Texture::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	// to copy the buffer into the image, we need the image to be in the right layout first
	// one way to perform layout transitions is the use image memory barrier
	VkCommandBuffer cmdBuff = m_CommandBuffers->BeginSingleTimeCommands();

	VkImageMemoryBarrier imgMemBarrier{};
	imgMemBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imgMemBarrier.oldLayout           = oldLayout; // use `VK_IMAGE_LAYOUT_UNDEFINED` as `oldLayout` if you don't care about the existing contents of the image
	imgMemBarrier.newLayout           = newLayout;
	imgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // use indices of the queues if you use barriers to transfer the ownership of the queue family
	imgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemBarrier.image               = image;
	// specific part of the image that is affected
	imgMemBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	imgMemBarrier.subresourceRange.baseMipLevel   = 0;
	imgMemBarrier.subresourceRange.levelCount     = 1;
	imgMemBarrier.subresourceRange.baseArrayLayer = 0;
	imgMemBarrier.subresourceRange.layerCount     = 1;

	// specify operations that the resource has to wait for
	// handle access masks and pipeline stages
	// to handle transfer writes that don't wait on anything
	// and shader reads, which should wait on transfer writes
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		imgMemBarrier.srcAccessMask = 0;
		imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else 
	{
		throw std::runtime_error("Unsupported layout transition!");
	}
	
	vkCmdPipelineBarrier(cmdBuff, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imgMemBarrier);

	m_CommandBuffers->EndSingleTimeCommands(cmdBuff);
}

// TODO: move this to image utils
void Texture::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer cmdBuff = m_CommandBuffers->BeginSingleTimeCommands();

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
	
	m_CommandBuffers->EndSingleTimeCommands(cmdBuff);
}

void Texture::CreateTextureImageView()
{
	m_TextureImageView = Swapchain::CreateImageView(m_Device->GetDevice(), m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Texture::CreateTextureSampler()
{
	VkPhysicalDeviceProperties phyDevProperties{};
	vkGetPhysicalDeviceProperties(m_Device->GetPhysicalDevice(), &phyDevProperties);

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy    = phyDevProperties.limits.maxSamplerAnisotropy;
	samplerCreateInfo.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // specifies which color is returned when sampling beyond the image with clamp to border addressing mode
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE; // specifies which coordinate system you want to use to address texels in an image. 
														  // If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth) and [0, texHeight) range
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp     = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(m_Device->GetDevice(), &samplerCreateInfo, nullptr, &m_TextureSampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Texture sampler!");
}

void Texture::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size        = size;
	bufferCreateInfo.usage       = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffers can be owned by a specific queue family or be shared between multiple at the same time

	if (vkCreateBuffer(m_Device->GetDevice(), &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create vertex buffer!");

	// assign memory to the buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_Device->GetDevice(), buffer, &memRequirements);

	// memory allocation
	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize  = memRequirements.size;
	memAllocInfo.memoryTypeIndex = Swapchain::FindMemoryType(m_Device->GetPhysicalDevice(), memRequirements.memoryTypeBits, properties);

	// we are not supposed to call vkAllocateMemory() for every individual buffer, because we have a limited maxMemoryAllocationCount
	// instead we can allocate a large memory and use offset to split the memory
	if (vkAllocateMemory(m_Device->GetDevice(), &memAllocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	
	vkBindBufferMemory(m_Device->GetDevice(), buffer, bufferMemory, 0);
}