#pragma once

#include <vulkan/vulkan.h>

#include "renderer/device.h"
#include "renderer/buffer/commandBuffer.h"


class Texture
{
public:
	Texture(const Device* device, const CommandBuffer* commandBuffers);
	~Texture();

	inline VkImageView GetImageView() const { return m_TextureImageView; }
	inline VkSampler GetSampler() const { return m_TextureSampler; }

private:
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

private:
	const Device* m_Device;
	const CommandBuffer* m_CommandBuffers;

	uint32_t m_MipLevels;
	VkImage m_TextureImage;
	VkDeviceMemory m_TextureImageMemory;
	VkImageView m_TextureImageView;
	VkSampler m_TextureSampler;
};