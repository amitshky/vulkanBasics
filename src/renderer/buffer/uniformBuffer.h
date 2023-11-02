#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "renderer/device.h"
#include "renderer/pipeline.h"
#include "renderer/texture.h"
#include "renderer/camera.h"


struct UniformBufferObject
{
	// explicitly speicify alignments
	// because vulkan expects structs to be in a specific alignment with the
	// structs in the shaders
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};


class UniformBuffer
{
public:
	UniformBuffer(const int maxFramesInFlight,
		const Device* device,
		const Pipeline* graphicsPipeline,
		const Texture* texture);
	~UniformBuffer();

	void Update(uint32_t currentFrameIdx, const Camera* camera);

	inline VkDescriptorSet& GetDescriptorSetAtIndex(const uint32_t index) { return m_DescriptorSets[index]; }
	inline VkDescriptorSet GetDescriptorSetAtIndex(const uint32_t index) const { return m_DescriptorSets[index]; }

private:
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

private:
	const int m_MaxFramesInFlight;
	const Device* m_Device;
	const Pipeline* m_GraphicsPipeline;
	const Texture* m_Texture;

	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformBuffersMemory;
	std::vector<void*> m_UniformBuffersMapped;

	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;
};