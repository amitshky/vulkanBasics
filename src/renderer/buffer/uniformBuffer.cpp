#include "uniformBuffer.h"

#include <stdexcept>
#include <chrono>
#include <array>

#include "renderer/swapchain.h"
#include "utils/bufferUtils.h"


UniformBuffer::UniformBuffer(const int maxFramesInFlight,
	const Device* device,
	const Pipeline* graphicsPipeline,
	const Texture* texture)
	: m_MaxFramesInFlight{ maxFramesInFlight },
	  m_Device{ device },
	  m_GraphicsPipeline{ graphicsPipeline },
	  m_Texture{ texture }
{
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
}

UniformBuffer::~UniformBuffer()
{
	for (size_t i = 0; i < m_MaxFramesInFlight; ++i)
	{
		vkDestroyBuffer(m_Device->GetDevice(), m_UniformBuffers[i], nullptr);
		vkFreeMemory(m_Device->GetDevice(), m_UniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(m_Device->GetDevice(), m_DescriptorPool, nullptr);
}

void UniformBuffer::CreateUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_UniformBuffers.resize(m_MaxFramesInFlight);
	m_UniformBuffersMemory.resize(m_MaxFramesInFlight);
	m_UniformBuffersMapped.resize(m_MaxFramesInFlight);

	for (size_t i = 0; i < m_MaxFramesInFlight; ++i)
	{
		utils::buff::CreateBuffer(m_Device->GetDevice(),
			m_Device->GetPhysicalDevice(),
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_UniformBuffers[i],
			m_UniformBuffersMemory[i]);

		vkMapMemory(m_Device->GetDevice(),
			m_UniformBuffersMemory[i],
			0,
			bufferSize,
			0,
			&m_UniformBuffersMapped[i]); // persistent mapping; pointer for
										 // application's lifetime
	}
}

void UniformBuffer::CreateDescriptorPool()
{
	// describe descriptor sets
	std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes{};
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(m_MaxFramesInFlight);
	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(m_MaxFramesInFlight);

	// allocate one for every frame
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
	descriptorPoolCreateInfo.maxSets =
		static_cast<uint32_t>(m_MaxFramesInFlight); // max descriptor sets that can be allocated

	if (vkCreateDescriptorPool(m_Device->GetDevice(), &descriptorPoolCreateInfo, nullptr, &m_DescriptorPool)
		!= VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool!");
}

void UniformBuffer::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(
		m_MaxFramesInFlight, m_GraphicsPipeline->GetDescriptorSetLayout());

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_DescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(m_MaxFramesInFlight);
	descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

	// we create one descriptor set for each frame with the same layout

	m_DescriptorSets.resize(m_MaxFramesInFlight);
	if (vkAllocateDescriptorSets(m_Device->GetDevice(), &descriptorSetAllocateInfo, m_DescriptorSets.data())
		!= VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets!");

	// configure descriptors in the descriptor sets
	for (size_t i = 0; i < m_MaxFramesInFlight; ++i)
	{
		// to configure descriptors that refer to buffers,
		// `VkDescriptorBufferInfo`
		VkDescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.buffer = m_UniformBuffers[i];
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo descriptorImageInfo{};
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfo.imageView = m_Texture->GetImageView();
		descriptorImageInfo.sampler = m_Texture->GetSampler();

		// to update the descriptor sets
		// we can update multiple descriptors at once in an array starting at
		// index dstArrayElement
		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_DescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1; // number of elements you want to update
		descriptorWrites[0].pBufferInfo = &descriptorBufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_DescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1; // number of elements you want to update
		descriptorWrites[1].pImageInfo = &descriptorImageInfo;

		// updates the configurations of the descriptor sets
		vkUpdateDescriptorSets(
			m_Device->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void UniformBuffer::Update(uint32_t currentFrameIdx, const Camera* camera)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f))
				* glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f))
				* glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.5f));
	ubo.view = camera->GetViewMatrix();
	ubo.proj = camera->GetProjectionMatrix();

	// copy the data from ubo to the uniform buffer (in GPU)
	memcpy(m_UniformBuffersMapped[currentFrameIdx], &ubo, sizeof(ubo));
}
