#pragma once

#include <optional>

#include "vulkanContext.h"
#include "core/vulkanConfig.h"


struct QueueFamilyIndices
{
	// std::optional contains no value until you assign something to it
	// we can check if it contains a value by calling has_value()
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	inline bool IsComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};


class Device
{
public:
	Device();
	Device(VkInstance vulkanInstance, VkSurfaceKHR windowSurface, const VulkanConfig* config);
	~Device();

	inline VkDevice GetDevice() const { return m_Device; }
	inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

	inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
	inline VkQueue GetPresentQueue() const { return m_GraphicsQueue; }
	
	// TODO: move this to swapchain class
	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice);
	// TODO: maybe make a separate Queue class
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice);

private:
	void PickPhysicalDevice();
	void CreateLogicalDevice();

	bool IsDeviceSuitable(VkPhysicalDevice physicalDevice);

	bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);

private:
	VkInstance m_VulkanInstance;
	VkSurfaceKHR m_WindowSurface;
	const VulkanConfig* m_Config;

	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;
};