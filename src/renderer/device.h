#pragma once

#include <optional>

#include "vulkanContext.h"
#include "core/vulkanConfig.h"


class Device
{
public:
	Device();
	Device(VkInstance vulkanInstance, VkSurfaceKHR windowSurface, const VulkanConfig* config);
	~Device();

	inline VkDevice GetDevice() const { return m_DeviceVk; }
	inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

	inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
	inline VkQueue GetPresentQueue() const { return m_GraphicsQueue; }
	
private:
	void PickPhysicalDevice();
	void CreateLogicalDevice();

	bool IsDeviceSuitable(VkPhysicalDevice physicalDevice);

	bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);

private:
	VkInstance m_VulkanInstance;
	VkSurfaceKHR m_WindowSurface;
	const VulkanConfig* m_Config;

	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_DeviceVk;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;
};