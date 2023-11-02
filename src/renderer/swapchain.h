#pragma once

#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "device.h"


class Swapchain
{
public:
	Swapchain(GLFWwindow* windowContext,
		const Device* device,
		VkSurfaceKHR windowSurface,
		VkSampleCountFlagBits msaaSamples);
	~Swapchain();

	void RecreateSwapchain();

	inline VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
	inline VkFormat GetSwapchainFormat() const { return m_SwapchainImageFormat; }
	inline VkExtent2D GetSwapchainExtent() const { return m_SwapchainExtent; }

	inline std::vector<VkImage> GetSwapchainImages() const { return m_SwapchainImages; }
	inline std::vector<VkImageView> GetSwapchainImageViews() const { return m_SwapchainImageViews; }

	inline uint32_t GetWidth() const { return m_SwapchainExtent.width; }
	inline uint32_t GetHeight() const { return m_SwapchainExtent.height; }

	inline VkRenderPass GetRenderPass() const { return m_RenderPass; }

	inline VkImage GetDepthImage() const { return m_DepthImage; }
	inline VkDeviceMemory GetDepthImageMemory() const { return m_DepthImageMemory; }
	inline VkImageView GetDepthImageView() const { return m_DepthImageView; }

	inline std::vector<VkFramebuffer> GetFramebuffers() const { return m_SwapchainFramebuffers; }
	inline VkFramebuffer GetFramebufferAtIndex(const uint32_t index) const { return m_SwapchainFramebuffers[index]; }

private:
	void CreateSwapchain();
	void CreateSwapchainImageViews();
	void CreateRenderPass();
	void CreateColorResources();
	void CreateDepthResources();
	void CreateFramebuffers();

	void CleanupSwapchain();

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& canditateFormats,
		VkImageTiling tiling,
		VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();
	bool HasStencilComponent(VkFormat format);

private:
	// constructor parameters
	GLFWwindow* m_WindowContext;
	const Device* m_Device;
	VkSurfaceKHR m_WindowSurface;
	VkSampleCountFlagBits m_MsaaSamples;

	VkSwapchainKHR m_Swapchain;
	std::vector<VkImage> m_SwapchainImages;
	VkFormat m_SwapchainImageFormat;
	VkExtent2D m_SwapchainExtent;
	std::vector<VkImageView> m_SwapchainImageViews;

	// TODO: make a framebuffer class
	VkRenderPass m_RenderPass;

	// for multisampling
	VkImage m_ColorImage;
	VkDeviceMemory m_ColorImageMemory;
	VkImageView m_ColorImageView;

	// TODO: make a depth buffer class
	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;

	// TODO: make a framebuffer class
	std::vector<VkFramebuffer> m_SwapchainFramebuffers;
};