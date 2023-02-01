#include "application.h"

#include <stdexcept>
#include <cstdint>
#include <set>
#include <iostream>
#include <algorithm>
#include <limits>
#include <fstream>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"


// TODO: pass objects like device (not VkDevice) and stuff not just specifically wht is needed like physicaldevice

// vertex data
std::vector<Vertex> vertices{
	{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } }, // index: 0; position: top-left;     color: red
	{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } }, // index: 1; position: top-right;    color: green
	{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // index: 2; position: bottom-right; color: blue
	{ { -0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }, // index: 3; position: bottom-left;  color: magenta

	{ { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } }, // index: 4; position: top-left;     color: red
	{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } }, // index: 5; position: top-right;    color: green
	{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // index: 6; position: bottom-right; color: blue
	{ { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }  // index: 7; position: bottom-left;  color: magenta
};

// indices for index buffer
std::vector<uint16_t> indices = { 
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};

const VulkanConfig config{
#ifdef NDEBUG // Release mode
	false,
#else         // Debug mode
	true,
#endif
	2,
	{
		"VK_LAYER_KHRONOS_validation"
	},
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	}
};


Application::Application(const char* title, int32_t width, int32_t height)
	: m_Config{&config}, 
	  m_Window{std::make_unique<Window>(title, width, height)}, 
	  m_VulkanContext{std::make_unique<VulkanContext>(title, m_Config)}, 
	  m_WindowSurface{std::make_unique<WindowSurface>(m_Window->GetWindowContext(), m_VulkanContext->GetInstance())},
	  m_Device{std::make_unique<Device>(m_VulkanContext->GetInstance(), m_WindowSurface->GetSurface(), m_Config)},
	  m_Swapchain{std::make_unique<Swapchain>(m_Window->GetWindowContext(), m_Device->GetDevice(), m_Device->GetPhysicalDevice(), m_WindowSurface->GetSurface())},
	  m_GraphicsPipeline{std::make_unique<Pipeline>(m_Device->GetDevice(), m_Swapchain->GetRenderPass())},
	  m_CommandBuffers{std::make_unique<CommandBuffers>(&config, m_WindowSurface->GetSurface(), m_Device->GetPhysicalDevice(), m_Device->GetDevice(), m_Device->GetGraphicsQueue())},
	  m_Camera{std::make_unique<Camera>(width / (float)height)}
{
	RegisterEvents();
	InitVulkan();
}

Application::~Application()
{
	Cleanup();
}

void Application::Run()
{
	float prevFrameRate = 0.0f;
	// TODO: change this to check for isRunning (member variable of this class)
	while (!glfwWindowShouldClose(m_Window->GetWindowContext()))
	{
		// calculating delta time
		float currentFrameTime = static_cast<float>(glfwGetTime());
		m_DeltaTime     = currentFrameTime - m_LastFrameTime;
		m_LastFrameTime = currentFrameTime;

		//printf("\r%8.2f fps", 1 / m_DeltaTime);

		m_Camera->OnUpdate(m_Window->GetWindowContext(), m_DeltaTime, m_Swapchain->GetWidth(), m_Swapchain->GetHeight());
		DrawFrame();

		ProcessInput();
		glfwPollEvents();
	}

	vkDeviceWaitIdle(m_Device->GetDevice());
}

void Application::InitVulkan()
{
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateSyncObjects();
}

void Application::RegisterEvents()
{
	glfwSetWindowUserPointer(m_Window->GetWindowContext(), this);

	glfwSetFramebufferSizeCallback(m_Window->GetWindowContext(), FramebufferResizeCallback);
	glfwSetCursorPosCallback(m_Window->GetWindowContext(), OnMouseMove);
}

void Application::Cleanup()
{
	vkDestroySampler(m_Device->GetDevice(), m_TextureSampler, nullptr);
	vkDestroyImageView(m_Device->GetDevice(), m_TextureImageView, nullptr);

	vkDestroyImage(m_Device->GetDevice(), m_TextureImage, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_TextureImageMemory, nullptr);

	for (size_t i = 0; i < m_Config->MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroyBuffer(m_Device->GetDevice(), m_UniformBuffers[i], nullptr);
		vkFreeMemory(m_Device->GetDevice(), m_UniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(m_Device->GetDevice(), m_DescriptorPool, nullptr);

	for (size_t i = 0; i < m_Config->MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_Device->GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device->GetDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_Device->GetDevice(), m_InFlightFences[i], nullptr);
	}

	// cleanup index buffer
	vkDestroyBuffer(m_Device->GetDevice(), m_IndexBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_IndexBufferMemory, nullptr);
	// cleanup vertex buffer
	vkDestroyBuffer(m_Device->GetDevice(), m_VertexBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_VertexBufferMemory, nullptr);
}

// TODO: move this to renderer class
void Application::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags            = 0; // how we are going to use the command buffer
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
		throw std::runtime_error("Failed to begin recording command buffer!");

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass        = m_Swapchain->GetRenderPass();
	renderPassBeginInfo.framebuffer       = m_Swapchain->GetFramebufferAtIndex(imageIndex);
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = m_Swapchain->GetSwapchainExtent();

	std::array<VkClearValue, 2> clearColor;
	clearColor[0].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
	clearColor[1].depthStencil = { 1.0f, 0 };
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearColor.size());
	renderPassBeginInfo.pClearValues    = clearColor.data();

	// the render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed
	// we don't use a second command buffers
	// start render pass
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetPipeline());
	VkViewport viewport{};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = static_cast<float>(m_Swapchain->GetWidth());
	viewport.height   = static_cast<float>(m_Swapchain->GetHeight());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_Swapchain->GetSwapchainExtent();
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// bind the vertex buffer
	VkBuffer vertexBuffers[] = { m_VertexBuffer };
	VkDeviceSize offsets[]   = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

	// descriptor sets are not unique to graphics or compute pipeline so we need to specify it
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetLayout(), 0, 1, &m_DescriptorSets[m_CurrentFrameIdx], 0, nullptr);

	// the draw command changes if index buffers are used
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

	// end render pass
	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to record command buffer!");
}

void Application::CreateSyncObjects()
{
	m_ImageAvailableSemaphores.resize(m_Config->MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(m_Config->MAX_FRAMES_IN_FLIGHT);
	m_InFlightFences.resize(m_Config->MAX_FRAMES_IN_FLIGHT);
	
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // create the fence in signaled state so that the firest frame doesnt have to wait

	// m_ImageAvailableSemaphore: used to acquire swapchain images
	// m_RenderFinishedSemaphore: signaled when command buffers have finished execution

	for (size_t i = 0; i < m_Config->MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(m_Device->GetDevice(), &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_Device->GetDevice(), &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS || 
			vkCreateFence(m_Device->GetDevice(), &fenceCreateInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create synchronization objects for a frame!");
	}
}

void Application::DrawFrame()
{
	// waiting for previous frame
	// but since the first frame doesnt have a previous frame to wait on, the fence is created in the signaled state
	vkWaitForFences(m_Device->GetDevice(), 1, &m_InFlightFences[m_CurrentFrameIdx], VK_TRUE, UINT64_MAX);

	// acquire image from the swapchain
	uint32_t imageIndex; // index of the next swapchain image
	VkResult result = vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain->GetSwapchain(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrameIdx], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) // swapchain is incompatible with the surface and cannot render
	{
		m_Swapchain->RecreateSwapchain();
		return; // we cannot simply return here, because the queue is never submitted and thus the fences are never signaled , causing a deadlock; to solve this we delay resetting the fences until after we check the swapchain
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) // if suboptimal, the swapchain can still be used to present but the surface properties are no longer the same; this is considered as success here
	{
		throw std::runtime_error("Failed to acquire swapchain image!");
	}

	// resetting the fence has been set after the result has been checked to avoid a deadlock
	// reset the fence to unsignaled state
	vkResetFences(m_Device->GetDevice(), 1, &m_InFlightFences[m_CurrentFrameIdx]);

	// record the command buffer
	m_CommandBuffers->ResetCommandBuffer(m_CurrentFrameIdx);
	RecordCommandBuffer(m_CommandBuffers->GetCommandBufferAtIndex(m_CurrentFrameIdx), imageIndex);

	UpdateUniformBuffer(m_CurrentFrameIdx);
	
	// submit the command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrameIdx] }; // semaphore to be waited before execution
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // stage of the pipeline to wait; wait writing colors to the image until it is available
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores    = waitSemaphores;
	submitInfo.pWaitDstStageMask  = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &m_CommandBuffers->GetCommandBufferAtIndex(m_CurrentFrameIdx); // command buffer to be submitted for execution
	VkSemaphore signalSemaphores[]  = { m_RenderFinishedSemaphores[m_CurrentFrameIdx] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = signalSemaphores;

	// signal the fence after executing the command buffer
	if (vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrameIdx]) != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer!");

	// submit the result back to the swapchain to render on the screen
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = signalSemaphores;

	// swapchain images and the indexes
	VkSwapchainKHR swapchains[] = { m_Swapchain->GetSwapchain() };
	presentInfo.swapchainCount  = 1;
	presentInfo.pSwapchains     = swapchains;
	presentInfo.pImageIndices   = &imageIndex;
	presentInfo.pResults        = nullptr; // check for every individual swapchain if presentation was successful

	// present the swapchain image
	result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

	// here both suboptimal and out-of-date are considered error and we recreate the swapchain because we want the best possible result
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
	{
		m_FramebufferResized = false;
		m_Swapchain->RecreateSwapchain();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swapchain image!");

	// increment current frame count
	m_CurrentFrameIdx = (m_CurrentFrameIdx + 1) % m_Config->MAX_FRAMES_IN_FLIGHT;
}

void Application::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	app->m_FramebufferResized = true;
}

void Application::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
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

void Application::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // source memory during transfer
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory); // staging buffer (in the CPU); temporary buffer

	// copy the vertex data to the buffer
	void* data;
	vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data); // mapping the buffer memory into CPU accessible memory
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // destination memory during transfer
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory); // the actual buffer (located in the device memory)

	CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

	vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);

	// the driver may not immediately copy the data into the buffer memory
	// so the writes to the buffer may not be visible yet
	// this is why we set VK_MEMORY_PROPERTY_HOST_COHERENT_BIT in the memory type
	// so that the mapped memory always matches the contents of the allocated memory
}

void Application::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer cmdBuff = m_CommandBuffers->BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size      = size;
	// transfer the contents of the buffers
	vkCmdCopyBuffer(cmdBuff, srcBuffer, dstBuffer, 1, &copyRegion);
	
	m_CommandBuffers->EndSingleTimeCommands(cmdBuff);
}

void Application::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // source memory during transfer
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory); // staging buffer (in the CPU); temporary buffer

	// copy the vertex data to the buffer
	void* data;
	vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data); // mapping the buffer memory into CPU accessible memory
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, // destination memory during transfer
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory); // the actual buffer (located in the device memory)

	CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

	vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);
}

void Application::CreateUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_UniformBuffers.resize(m_Config->MAX_FRAMES_IN_FLIGHT);
	m_UniformBuffersMemory.resize(m_Config->MAX_FRAMES_IN_FLIGHT);
	m_UniformBuffersMapped.resize(m_Config->MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < m_Config->MAX_FRAMES_IN_FLIGHT; ++i)
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[i], m_UniformBuffersMemory[i]);

		//                    // actual buffer in GPU                      // buffer mapped to the CPU
		vkMapMemory(m_Device->GetDevice(), m_UniformBuffersMemory[i], 0, bufferSize, 0, &m_UniformBuffersMapped[i]); // persistent mapping; pointer for application's lifetime
	}
}

void Application::UpdateUniformBuffer(uint32_t currentFrameIdx)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view  = m_Camera->GetViewMatrix();
	ubo.proj  = m_Camera->GetProjectionMatrix();
	//ubo.proj[1][1] *= -1; // glm was designed for opengl where the y-coord for clip coordinate is flipped

	// copy the data from ubo to the uniform buffer (in GPU)
	memcpy(m_UniformBuffersMapped[currentFrameIdx], &ubo, sizeof(ubo));
}

void Application::CreateDescriptorPool()
{
	// describe descriptor sets
	std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes{};
	descriptorPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(m_Config->MAX_FRAMES_IN_FLIGHT);
	descriptorPoolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(m_Config->MAX_FRAMES_IN_FLIGHT);

	// allocate one for every frame
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes    = descriptorPoolSizes.data();
	descriptorPoolCreateInfo.maxSets       = static_cast<uint32_t>(m_Config->MAX_FRAMES_IN_FLIGHT); // max descriptor sets that can be allocated

	if (vkCreateDescriptorPool(m_Device->GetDevice(), &descriptorPoolCreateInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool!");
}

void Application::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(m_Config->MAX_FRAMES_IN_FLIGHT, m_GraphicsPipeline->GetDescriptorSetLayout());
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool     = m_DescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(m_Config->MAX_FRAMES_IN_FLIGHT);
	descriptorSetAllocateInfo.pSetLayouts        = descriptorSetLayouts.data();

	// we create one descriptor set for each frame with the same layout

	m_DescriptorSets.resize(m_Config->MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(m_Device->GetDevice(), &descriptorSetAllocateInfo, m_DescriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets!");

	// configure descriptors in the descriptor sets
	for (size_t i = 0; i < m_Config->MAX_FRAMES_IN_FLIGHT; ++i)
	{
		// to configure descriptors that refer to buffers, `VkDescriptorBufferInfo`
		VkDescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.buffer = m_UniformBuffers[i];
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range  = sizeof(UniformBufferObject);

		VkDescriptorImageInfo descriptorImageInfo{};
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfo.imageView   = m_TextureImageView;
		descriptorImageInfo.sampler     = m_TextureSampler;

		// to update the descriptor sets
		// we can update multiple descriptors at once in an array starting at index dstArrayElement
		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet          = m_DescriptorSets[i];
		descriptorWrites[0].dstBinding      = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1; // number of elements you want to update
		descriptorWrites[0].pBufferInfo     = &descriptorBufferInfo;

		descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet          = m_DescriptorSets[i];
		descriptorWrites[1].dstBinding      = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1; // number of elements you want to update
		descriptorWrites[1].pImageInfo     = &descriptorImageInfo;
		
		// updates the configurations of the descriptor sets
		vkUpdateDescriptorSets(m_Device->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Application::OnMouseMove(GLFWwindow* window, double xpos, double ypos)
{
	auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	app->m_Camera->OnMouseMove(window, xpos, ypos);
}

void Application::ProcessInput()
{
	// close window
	if (glfwGetKey(m_Window->GetWindowContext(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(m_Window->GetWindowContext(), true);

	// mouse button 1 to move camera
	// hide cursor when moving camera
	if (glfwGetMouseButton(m_Window->GetWindowContext(), GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
		glfwSetInputMode(m_Window->GetWindowContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// unhide cursor when camera stops moving
	else if (glfwGetMouseButton(m_Window->GetWindowContext(), GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE)
		glfwSetInputMode(m_Window->GetWindowContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Application::CreateTextureImage()
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

void Application::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
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

void Application::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
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

void Application::CreateTextureImageView()
{
	m_TextureImageView = Swapchain::CreateImageView(m_Device->GetDevice(), m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Application::CreateTextureSampler()
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
