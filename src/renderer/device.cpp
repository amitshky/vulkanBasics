#include "device.h"

#include <stdexcept>
#include <iostream>
#include <set>


Device::Device()
	: m_VulkanInstance{nullptr}, m_WindowSurface{nullptr}, m_Config{nullptr}
{

}

Device::Device(VkInstance vulkanInstance, VkSurfaceKHR windowSurface, const VulkanConfig* config)
	: m_VulkanInstance{vulkanInstance}, m_WindowSurface{windowSurface}, m_Config{config}
{
	PickPhysicalDevice();
	CreateLogicalDevice();
}

Device::~Device()
{
	vkDestroyDevice(m_Device, nullptr);
}

void Device::PickPhysicalDevice()
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, nullptr); // get physical device count

	if (physicalDeviceCount == 0)
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> physicalDevices{physicalDeviceCount};
	vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, physicalDevices.data());

	// currently we only work with one device
	for (const auto& device : physicalDevices)
	{
		if (IsDeviceSuitable(device))
		{
			m_PhysicalDevice = device;
			break;
		}
	}

	if (m_PhysicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to find a suitable GPU!");

	// we dont need to write this
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

	std::cout << "Physical device info:\n"
			  << "    Device name: " << physicalDeviceProperties.deviceName << "\n\n";
}

void Device::CreateLogicalDevice()
{
	// create queue
	QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	// we have multiple queues so we create a set of unique queue families
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};
	
	float queuePriority = 1.0f;
	for (const auto& queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount       = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// specify used device features
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	
	// create logical device
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos    = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures     = &deviceFeatures;

	// these are similar to create instance but they are device specific this time
	deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(m_Config->deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = m_Config->deviceExtensions.data();

	if (m_Config->enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(m_Config->validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = m_Config->validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
		throw std::runtime_error("Failed to create logcial device!");

	// get the queue handle
	vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, indices.presentFamily.value(),  0, &m_PresentQueue);
}

bool Device::IsDeviceSuitable(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndices indicies = FindQueueFamilies(physicalDevice);

	// checking for extension availability like swapchain extension availability
	bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);

	// checking if swapchain is supported by window surface
	bool swapchainAdequate = false;
	if (extensionsSupported)
	{
		SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(physicalDevice);
		swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return indicies.IsComplete() && extensionsSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices Device::FindQueueFamilies(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies{queueFamilyCount};
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	// find a queue that supports graphics commands
	for (int i = 0; i < queueFamilies.size(); ++i)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;
		
		// check for queue family compatible for presentation
		// the graphics queue and the presentation queue might end up being the same
		// but we treat them as separate queues
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_WindowSurface, &presentSupport);

		if (presentSupport)
			indices.presentFamily = i;
		
		if (indices.IsComplete())
			break;
	}

	return indices;
}

bool Device::CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions{extensionCount};
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions{m_Config->deviceExtensions.begin(), m_Config->deviceExtensions.end()};

	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

SwapchainSupportDetails Device::QuerySwapchainSupport(VkPhysicalDevice physicalDevice)
{
	// Simply checking swapchain availability is not enough, 
	// we need to check if it is supported by our window surface or not
	// We need to check for:
	// * basic surface capabilities (min/max number of images in swap chain)
	// * surface formats (pixel format and color space)
	// * available presentation modes
	SwapchainSupportDetails swapchainDetails{};

	// query surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_WindowSurface, &swapchainDetails.capabilities);
	// query surface format
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_WindowSurface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		swapchainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_WindowSurface, &formatCount, swapchainDetails.formats.data());
	}
	// query supported presentation modes
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_WindowSurface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		swapchainDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_WindowSurface, &presentModeCount, swapchainDetails.presentModes.data());
	}

	return swapchainDetails;
}