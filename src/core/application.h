#pragma once

#include <vector>
#include <array>
#include <optional>
#include <chrono>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "core/window.h"
#include "core/vulkanConfig.h"

#include "renderer/vulkanContext.h"
#include "renderer/windowSurface.h"
#include "renderer/device.h"
#include "renderer/swapchain.h"
#include "renderer/pipeline.h"
#include "renderer/texture.h"

#include "renderer/buffer/commandBuffer.h"
#include "renderer/buffer/vertexBuffer.h"
#include "renderer/buffer/indexBuffer.h"
#include "renderer/buffer/uniformBuffer.h"

#include "renderer/camera.h"


class Application
{
public:
	Application(const char* title, int32_t width, int32_t height);
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void Run();

private:
	void InitVulkan();
	void RegisterEvents();
	void Cleanup();

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void CreateSyncObjects();
	void DrawFrame();

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	void ProcessInput();
	static void OnMouseMove(GLFWwindow* window, double xpos, double ypos);

private:
	const VulkanConfig* m_Config;

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<VulkanContext> m_VulkanContext;
	std::unique_ptr<WindowSurface> m_WindowSurface;
	
	std::unique_ptr<Device> m_Device;
	std::unique_ptr<Swapchain> m_Swapchain;
	std::unique_ptr<Pipeline> m_GraphicsPipeline;

	std::unique_ptr<CommandBuffer> m_CommandBuffers;
	std::unique_ptr<VertexBuffer> m_VertexBuffer;
	std::unique_ptr<IndexBuffer> m_IndexBuffer;

	std::unique_ptr<Texture> m_Texture;
	std::unique_ptr<UniformBuffer> m_UniformBuffers;
	
	std::unique_ptr<Camera> m_Camera;

	// synchronization objects
	// semaphores to sync gpu operations and fence to sync cpu operation with the gpu operation
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;

	// frames in-flight
	uint32_t m_CurrentFrameIdx = 0;

	// check for resize
	bool m_FramebufferResized = false;
	
	// for delta time
	float m_LastFrameTime = 0.0f;
	float m_DeltaTime     = 0.0f;
};
