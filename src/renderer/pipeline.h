#pragma once

#include <array>

#include <vulkan/vulkan.h>
#include "glm/glm.hpp"


class Pipeline
{
public:
	Pipeline(VkDevice deviceVk, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples);
	~Pipeline();

	inline VkPipeline GetPipeline() const { return m_Pipeline; }
	inline VkPipelineLayout GetLayout() const { return m_PipelineLayout; }

	inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

private:
	void CreateGraphicsPipeline();
	void CreateDescriptorSetLayout(); // TODO: move this to Uniform buffers or
									  // descriptor class

private:
	VkDevice m_DeviceVk;
	VkRenderPass m_RenderPass;
	VkSampleCountFlagBits m_MsaaSamples;

	VkDescriptorSetLayout m_DescriptorSetLayout;

	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_Pipeline;
};