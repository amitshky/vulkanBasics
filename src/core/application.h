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
#include "renderer/camera.h"


struct UniformBufferObject 
{
	// explicitly speicify alignments
	// because vulkan expects structs to be in a specific alignment with the structs in the shaders
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};


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

	void CreateCommandPool();
	void CreateCommandBuffer();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void CreateSyncObjects();
	void DrawFrame();

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer cmdBuff);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentFrameIdx);
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void ProcessInput();
	static void OnMouseMove(GLFWwindow* window, double xpos, double ypos);

	void CreateTextureImage();
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void CreateTextureImageView();
	void CreateTextureSampler();

private:
	const VulkanConfig* m_Config;

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<VulkanContext> m_VulkanContext;
	std::unique_ptr<WindowSurface> m_WindowSurface;
	
	std::unique_ptr<Device> m_Device;
	std::unique_ptr<Swapchain> m_Swapchain;
	std::unique_ptr<Pipeline> m_GraphicsPipeline;
	
	std::unique_ptr<Camera> m_Camera;

	// uniform buffer descriptor layout
	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformBuffersMemory;
	std::vector<void*> m_UniformBuffersMapped;
	// descriptor pool
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	// command buffer
	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	// synchronization objects
	// semaphores to sync gpu operations and fence to sync cpu operation with the gpu operation
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;

	// frames in-flight
	uint32_t m_CurrentFrameIdx = 0;

	// check for resize
	bool m_FramebufferResized = false;
	
	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_VertexBufferMemory;
	VkBuffer m_IndexBuffer;
	VkDeviceMemory m_IndexBufferMemory;

	// for delta time
	float m_LastFrameTime = 0.0f;
	float m_DeltaTime     = 0.0f;

	// textures
	VkImage m_TextureImage;
	VkDeviceMemory m_TextureImageMemory;
	VkImageView m_TextureImageView;
	VkSampler m_TextureSampler;
};
