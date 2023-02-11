#include "application.h"

#include <stdexcept>
#include <cstdint>
#include <set>
#include <iostream>
#include <algorithm>
#include <limits>
#include <fstream>
#include <cmath>


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
	  m_Swapchain{std::make_unique<Swapchain>(m_Window->GetWindowContext(), m_Device.get(), m_WindowSurface->GetSurface())},
	  m_GraphicsPipeline{std::make_unique<Pipeline>(m_Device->GetDevice(), m_Swapchain->GetRenderPass())},
	  m_CommandBuffers{std::make_unique<CommandBuffer>(config.MAX_FRAMES_IN_FLIGHT, m_WindowSurface->GetSurface(), m_Device.get())},
	  m_VertexBuffer{std::make_unique<VertexBuffer>(m_Device.get(), m_CommandBuffers.get(), vertices)},
	  m_IndexBuffer{std::make_unique<IndexBuffer>(m_Device.get(), m_CommandBuffers.get(), indices)},
	  m_Texture{std::make_unique<Texture>(m_Device.get(), m_CommandBuffers.get())},
	  m_UniformBuffers{std::make_unique<UniformBuffer>(config.MAX_FRAMES_IN_FLIGHT, m_Device.get(), m_GraphicsPipeline.get(), m_Texture.get())},
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
	for (size_t i = 0; i < m_Config->MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_Device->GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device->GetDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_Device->GetDevice(), m_InFlightFences[i], nullptr);
	}
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

	// TODO: think on where bind functions should go (like should they be in their respective classes such as vertexbuffer or somewhrer outside the class)
	// TODO: maybe create something similar to a vertex array where you can define the vertex buffers and index buffers and bind them when needed

	// bind the vertex buffer
	VkBuffer vertexBuffers[] = { m_VertexBuffer->GetVertexBuffer() };
	VkDeviceSize offsets[]   = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);

	// descriptor sets are not unique to graphics or compute pipeline so we need to specify it
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetLayout(), 0, 1, &m_UniformBuffers->GetDescriptorSetAtIndex(m_CurrentFrameIdx), 0, nullptr);

	// the draw command changes if index buffers are used
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

	// end render pass
	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to record command buffer!");
}

// TODO: make a SyncObjects class
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
	uint32_t nextImageIndex; // index of the next swapchain image
	VkResult result = vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain->GetSwapchain(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrameIdx], VK_NULL_HANDLE, &nextImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) // swapchain is incompatible with the surface and cannot render
	{
		m_Swapchain->RecreateSwapchain();
		return; // we cannot simply return here, because the queue is never submitted and thus the fences are never signaled , causing a deadlock; to solve this we delay resetting the fences until after we check the swapchain
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) // if suboptimal, the swapchain can still be used to present but the surface properties are no longer the same; this is considered as success here
		throw std::runtime_error("Failed to acquire swapchain image!");

	// resetting the fence has been set after the result has been checked to avoid a deadlock
	// reset the fence to unsignaled state
	vkResetFences(m_Device->GetDevice(), 1, &m_InFlightFences[m_CurrentFrameIdx]);

	// record the command buffer
	m_CommandBuffers->ResetCommandBuffer(m_CurrentFrameIdx);
	RecordCommandBuffer(m_CommandBuffers->GetCommandBufferAtIndex(m_CurrentFrameIdx), nextImageIndex);

	m_UniformBuffers->Update(m_CurrentFrameIdx, m_Camera.get());
	
	// TODO: abstract queue submit (prolly in Device or Queue class)
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
	presentInfo.pImageIndices   = &nextImageIndex;
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
