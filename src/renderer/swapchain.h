#pragma once

#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "device.h"


class Swapchain
{
public:
	// TODO: try if passing a const ref of thw Window class itself would work (because the obj is a unique ptr)
	// and also make the static functions in the device class into regular member functions
	Swapchain(GLFWwindow* windowContext, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface);
	~Swapchain();

	void RecreateSwapchain();

	static void CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, 
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

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
	inline VkFramebuffer GetFramebufferAtIndex(const int index) const { return m_SwapchainFramebuffers[index]; }

private:
	void CreateSwapchain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateDepthResources();
	void CreateFramebuffers();

	void CleanupSwapchain();

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& canditateFormats, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();
	bool HasStencilComponent(VkFormat format);

private:
	GLFWwindow* m_WindowContext;
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	VkSurfaceKHR m_WindowSurface;

	VkSwapchainKHR m_Swapchain;
	std::vector<VkImage> m_SwapchainImages;
	VkFormat m_SwapchainImageFormat;
	VkExtent2D m_SwapchainExtent;
	std::vector<VkImageView> m_SwapchainImageViews;

	VkRenderPass m_RenderPass;

	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;

	std::vector<VkFramebuffer> m_SwapchainFramebuffers;
};