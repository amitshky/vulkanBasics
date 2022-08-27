#include "application.hpp"

#include <stdexcept>
#include <cstdint>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include <limits>
#include <fstream>


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pDebugMessenger
)
{
	// requires a valid instance to have been created
	// so right now we cannot debug any issues in vkCreateInstance
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"); // returns nullptr if the function couldn't be loaded
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	// must be destroyed before instance is destroyed
	// so right now we cannot debug any issues in vkDestroyInstance
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}


Application::Application(const std::string& title, int32_t width, int32_t height)
	: m_Title(title), m_Width(width), m_Height(height)
{
	InitWindow();
	InitVulkan();
}

Application::~Application()
{
	Cleanup();
}

void Application::Run()
{
	while (!glfwWindowShouldClose(m_Window))
	{
		glfwPollEvents();
	}
}

void Application::InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);
}

void Application::InitVulkan()
{
	CreateVulkanInstance();
	SetupDebugMessenger();
	CreateWindowSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapchain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffer();
	CreateCommandPool();
	CreateCommandBuffer();
}

void Application::Cleanup()
{
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

	for (auto framebuffer : m_SwapchainFramebuffer)
		vkDestroyFramebuffer(m_Device, framebuffer, nullptr);

	vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

	for (const auto& imageView : m_SwapchainImageviews)
		vkDestroyImageView(m_Device, imageView, nullptr);

	vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
	vkDestroyDevice(m_Device, nullptr);

	if (enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);

	vkDestroySurfaceKHR(m_VulkanInstance, m_WindowSurface, nullptr);
	vkDestroyInstance(m_VulkanInstance, nullptr);

	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Application::CreateVulkanInstance()
{
	if (enableValidationLayers && !CheckValidationLayerSupport())
		throw std::runtime_error("Validation layers requested, but not available!");

	// info about our application
	VkApplicationInfo appInfo{};
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = m_Title.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "No Engine";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;

	// specify which extensions and validation layers to use
	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	// we need extensions to interface with the window
	uint32_t extensionCount = 0;
	auto extensions = GetRequiredExtensions();  // returns a required list of extensions based on whether validation layers are enabled or not
	instanceCreateInfo.enabledExtensionCount   = extensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	// debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	// specify global validation layers
	if (enableValidationLayers) // include validation layers if enabled
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugMessengerInfo);
		instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessengerInfo;
	} 
	else
	{
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.pNext = nullptr;
	}

	// to check the available extensions
	// we dont need this for now
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions{extensionCount};
	// query extension details
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
	std::cout << "Available Vulkan extensions (" << extensionCount << "):\n";
	for (auto& extension : availableExtensions)
		std::cout << "    " << extension.extensionName << '\n';
	std::cout << '\n';


	// create an instance
	if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan Instance!");
}

bool Application::CheckValidationLayerSupport()
{
	// check if all of the requested layers are available
	uint32_t validationLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers{validationLayerCount};
	vkEnumerateInstanceLayerProperties(&validationLayerCount, availableLayers.data());
	
	for (const auto& layer : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (std::strcmp(layer, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}

std::vector<const char*> Application::GetRequiredExtensions()
{
	uint32_t extensionCount = 0;
	const char** extensions; // GLFW extensions
	extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	std::vector<const char*> availableExtensions{extensions, extensions + extensionCount};

	if (enableValidationLayers)
		availableExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	
	return availableExtensions;
}

// the returned value indicates if the Vulkan call that triggered the validation layer message should be aborted
// if true, the call is aborted with `VK_ERROR_VALIDATION_FAILED_EXT` error
VKAPI_ATTR VkBool32 VKAPI_CALL Application::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,    // the enums are of this are set up in such a way that we can compare using comparision operator 
															   // to check the severity of the message
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, // contains the actual details of the message
	void* pUserData                                            // allows you to pass your own data
)
{
	std::cerr << "Validation layer: " << pCallbackData->pMessage << '\n';
	return VK_FALSE;
}

void Application::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo) 
{
	// debug messenger provides explicit control over what kind of messages to log
	debugMessengerInfo = {};
	debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
									   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
									//   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
	debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
								   | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
								   | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
	debugMessengerInfo.pfnUserCallback = DebugCallback; // call back function for debug messenger
	debugMessengerInfo.pUserData = nullptr; // Optional
}

void Application::SetupDebugMessenger()
{
	if (!enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	PopulateDebugMessengerCreateInfo(debugMessengerInfo);

	if (CreateDebugUtilsMessengerEXT(m_VulkanInstance, &debugMessengerInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
		throw std::runtime_error("Failed to setup debug messenger!");
}

void Application::PickPhysicalDevice()
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, nullptr);

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
	
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &physicalDeviceFeatures);

	std::cout << "Physical device info:\n"
			  << "    Device name: " << physicalDeviceProperties.deviceName << "\n\n";
}

bool Application::IsDeviceSuitable(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndices indicies = FindQueueFamilies(physicalDevice);

	// checking for extension availability like swap chain extension availability
	bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);

	// checking if swap chain supported by window surface
	bool swapchainAdequate = false;
	if (extensionsSupported)
	{
		SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(physicalDevice);
		swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
	}

	return indicies.IsComplete() && extensionsSupported && swapchainAdequate;
}

QueueFamilyIndices Application::FindQueueFamilies(VkPhysicalDevice physicalDevice)
{
	// for now we only look for queue that supports graphics commands
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

void Application::CreateLogicalDevice()
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
	for (const auto& queueFamily: uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount       = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// specify used device features
	// currently we don't need anything special
	VkPhysicalDeviceFeatures deviceFeatures{};
	
	// create logical device
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos    = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pEnabledFeatures     = &deviceFeatures;

	// these are similar to create instance but they are device specific this time
	deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledExtensionCount = 0;
	}

	if (vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
		throw std::runtime_error("Failed to create logcial device!");

	// get the queue handle
	vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, indices.presentFamily.value(),  0, &m_PresentQueue);
}

void Application::CreateWindowSurface()
{
	if (glfwCreateWindowSurface(m_VulkanInstance, m_Window, nullptr, &m_WindowSurface) != VK_SUCCESS)
		throw std::runtime_error("Failed to create window surface!");
}

bool Application::CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions{extensionCount};
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions{deviceExtensions.begin(), deviceExtensions.end()};

	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

SwapchainSupportDetails Application::QuerySwapchainSupport(VkPhysicalDevice physicalDevice)
{
	// Simply checking swap chain availability is not enough, 
	// we need to check if it is supported by our window surface or not
	// We need to check for:
	// * basic surface capabilities (min/max number of images in swap chain)
	// * surface formats (pixel format and color space)
	// * available presentation modes
	SwapchainSupportDetails swapchainDetails;

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

VkSurfaceFormatKHR Application::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	// VkSurfaceFormatKHR contains format and colorspace
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	// normally we need to rank the available formats
	return availableFormats[0];
}

VkPresentModeKHR Application::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes) 
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	// resolution of window and the swap chain image not equal
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else
	{
		int width = 0;
		int height = 0;
		// screen coordinates might not correspond to pixels in high DPI displays so we cannot just use the original width and height
		glfwGetFramebufferSize(m_Window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		// bind the width and height to allowed range
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.width = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	};
}

void Application::CreateSwapchain()
{
	SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(m_PhysicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR   presentMode   = ChooseSwapPresentMode(swapchainSupport.presentModes);
	VkExtent2D         extent        = ChooseSwapExtent(swapchainSupport.capabilities);

	// specify how many images we want in the swapchain
	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
	if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
		imageCount = swapchainSupport.capabilities.maxImageCount;

	// create swapchain
	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = m_WindowSurface;

	// swapchain image details
	swapchainCreateInfo.minImageCount    = imageCount;
	swapchainCreateInfo.imageFormat      = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent      = extent;
	swapchainCreateInfo.imageArrayLayers = 1; // number of layers in each image
	swapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// handle swapchain images across multiple queue families
	QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;  // image used across multiple queue families
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;  //image owned by one queue family at a time
		swapchainCreateInfo.queueFamilyIndexCount = 0;                          // optional
		swapchainCreateInfo.pQueueFamilyIndices   = 0;                          // optional
	}

	swapchainCreateInfo.preTransform   = swapchainSupport.capabilities.currentTransform;  // we can rotate, flip, etc
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;               // specify if alpha channel is used for blending
	swapchainCreateInfo.presentMode    = presentMode;
	swapchainCreateInfo.clipped        = VK_TRUE;
	swapchainCreateInfo.oldSwapchain   = VK_NULL_HANDLE;                                  // if new swapchain is to be created, the old one should be referenced here

	if (vkCreateSwapchainKHR(m_Device, &swapchainCreateInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Swap chain!");

	// get swapchain images
	vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
	m_SwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());

	m_SwapchainImageFormat = surfaceFormat.format;
	m_SwapchainExtent = extent;
}

void Application::CreateImageViews()
{
	m_SwapchainImageviews.resize(m_SwapchainImages.size());

	for (int i = 0; i < m_SwapchainImages.size(); ++i)
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = m_SwapchainImages[i];
		// specify how image data should be interpreted
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // treat images as 2D textures
		imageViewCreateInfo.format   = m_SwapchainImageFormat;

		// here you can map the channels as you wish; for example map all the channels to red for a monochrome texture
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// describe the image's purpose and which part of the image to access
		// our image will be used as color targets without any mipmapping levels or multiple layers
		imageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
		imageViewCreateInfo.subresourceRange.levelCount     = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount     = 1;

		if (vkCreateImageView(m_Device, &imageViewCreateInfo, nullptr, &m_SwapchainImageviews[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create Image views!");
	}
}

void Application::CreateRenderPass()
{
	// color attachment
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format  = m_SwapchainImageFormat; // the format should match the format of the swapchain
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;  // we dont have multisampling yet
	// determining what to do with the data in the attachment
	// for color and depth data
	colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// for stencil data
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// the textures and framebuffer are represented by `VkImage`, with a certain pixel format
	// the layout of the pixel format can be changed based on what you're trying to do
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;        // we don't care about the previous layout of the image
	colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // the image should be ready for presentation using swapchain

	// attachment references
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; // index of attachment ot reference
	colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	// subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	// the index of the attachment in this array is directly referenced from the fragment shader // layout (location=0) out vec4 outColor
	subpass.pColorAttachments    = &colorAttachmentRef;

	// render pass
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments    = &colorAttachment;
	renderPassCreateInfo.subpassCount    = 1;
	renderPassCreateInfo.pSubpasses      = &subpass;

	if (vkCreateRenderPass(m_Device, &renderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass!");
}

void Application::CreateGraphicsPipeline()
{
	// load the SPIR-V file
	auto vertexShaderCode   = LoadShader("assets/shaders/gradientTriangle.vert.spv");
	auto fragmentShaderCode = LoadShader("assets/shaders/gradientTriangle.frag.spv");

	// creating shader modules
	VkShaderModule vertexShaderModule   = CreateShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

	// shader stage creation
	// filling in the structure for the vertex shader
	VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo{};
	vertexShaderStageCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageCreateInfo.module = vertexShaderModule;
	vertexShaderStageCreateInfo.pName  = "main"; // entrypoint // so it is possible to combine multiple shaders into a single module

	// filling in the structure for the fragment shader
	VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{};
	fragmentShaderStageCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageCreateInfo.module = fragmentShaderModule;
	fragmentShaderStageCreateInfo.pName  = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

	// fixed functions
	// vertex input
	// we hard-coded the vertex data so we specify that there is not vertex data for now
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
	vertexInputCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount   = 0;
	vertexInputCreateInfo.pVertexBindingDescriptions      = nullptr;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputCreateInfo.pVertexAttributeDescriptions    = nullptr;

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	// viewport
	VkViewport viewport{};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = static_cast<float>(m_SwapchainExtent.width);
	viewport.height   = static_cast<float>(m_SwapchainExtent.width);
	viewport.minDepth = 0.0f; // range of depth values for framebuffer
	viewport.maxDepth = 1.0f;

	// scissor
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_SwapchainExtent;

	// viewport state
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports    = &viewport;
	viewportStateCreateInfo.scissorCount  = 1;
	viewportStateCreateInfo.pScissors     = &scissor;

	// rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
	rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;                // if true, clamp to depth instead of discarding
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;                // if true, the geometry never passes through rasterizer stage
	rasterizationStateCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;    // how fragments are generated
	rasterizationStateCreateInfo.lineWidth               = 1.0f;                    // thickness of lines
	rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;   // type of face culling
	rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_CLOCKWISE; // vertex order for faces to be considered front-face
	// the depth value can be altered by adding a constant value based on fragment slope
	rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp          = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor    = 0.0f;

	// multisampling
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
	multisampleStateCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable   = VK_FALSE;
	multisampleStateCreateInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading      = 1.0f;
	multisampleStateCreateInfo.pSampleMask           = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable      = VK_FALSE;

	// no depth and stencil testing for now

	// color blending
	// configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable         = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	// the above config does the following
	// finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
	// finalColor.a = newAlpha.a;

	// dynamic states
	// allows you to specify the data at drawing time
	std::vector<VkDynamicState> dynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates    = dynamicStates.data();

	// configuration for global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
	colorBlendStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable     = VK_FALSE;
	colorBlendStateCreateInfo.logicOp           = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount   = 1;
	colorBlendStateCreateInfo.pAttachments      = &colorBlendAttachment;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

	// pipeline layout
	// specify uniforms
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount         = 0;
	pipelineLayoutCreateInfo.pSetLayouts            = nullptr;
	// push constants are another way of passing dynamic values to the shaders
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;

	if (vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline!");


	// graphics pipeline
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// config all previos objects
	graphicsPipelineCreateInfo.stageCount          = 2;
	graphicsPipelineCreateInfo.pStages             = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState   = &vertexInputCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState   = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState  = nullptr;
	graphicsPipelineCreateInfo.pColorBlendState    = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState       = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout              = m_PipelineLayout;
	graphicsPipelineCreateInfo.renderPass          = m_RenderPass;
	graphicsPipelineCreateInfo.subpass             = 0;
	// pipeline can be created by derivin from a previous pipeline
	// less expensive to set up a pipeline if it has common functionality with an existing pipeline
	graphicsPipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex   = -1;

	// multiple graphicsPipelineCreateInfo can be passed
	// pipelineCache (2nd param) can be used to store and reuse data
	if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline!");


	// once the pipeline creation is complete, the shader modules can be destroyed
	vkDestroyShaderModule(m_Device, vertexShaderModule,   nullptr);
	vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
}

std::vector<char> Application::LoadShader(const std::string& filepath)
{
	std::ifstream file{ filepath, std::ios::binary | std::ios::ate };
	if (!file.is_open())
		throw std::runtime_error("Error opening shader file: " + filepath);

	const size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkShaderModule Application::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	// code is in char but shaderModule expects it to be in uint32_t
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_Device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module!");
	
	return shaderModule;
}

void Application::CreateFramebuffer()
{
	m_SwapchainFramebuffer.resize(m_SwapchainImageviews.size());

	for (size_t i = 0; i < m_SwapchainImageviews.size(); ++i)
	{
		VkImageView attachment[] = {
			m_SwapchainImageviews[i]
		};

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass      = m_RenderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments    = attachment;
		framebufferCreateInfo.width           = m_SwapchainExtent.width;
		framebufferCreateInfo.height          = m_SwapchainExtent.height;
		framebufferCreateInfo.layers          = 1;

		if (vkCreateFramebuffer(m_Device, &framebufferCreateInfo, nullptr, &m_SwapchainFramebuffer[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create framebuffer!");
	}
}

void Application::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;  // allows command buffers to be recorded individually
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();        // command pools can only allocate command buffers that are submitted by a single type of queue

	if (vkCreateCommandPool(m_Device, &commandPoolCreateInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pools!");
}

void Application::CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool        = m_CommandPool;
	commandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // can be submitted to a queue for execution, but cannot be called from other command buffers
	commandBufferAllocateInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(m_Device, &commandBufferAllocateInfo, &m_CommandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command buffer allocator info!");
}

void Application::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags            = 0; // how we are going to use the command buffer
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(m_CommandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
		throw std::runtime_error("Failed to begin recording command buffer!");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass        = m_RenderPass;
	renderPassInfo.framebuffer       = m_SwapchainFramebuffer[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_SwapchainExtent;

	VkClearValue clearColor          = {{{ 0.01f, 0.01f, 0.01f, 1.0f }}};
	renderPassInfo.clearValueCount   = 1;
	renderPassInfo.pClearValues      = &clearColor;

	// the render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed
	// we don't use a second command buffers
	// start render pass
	vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	VkViewport viewport{};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = static_cast<float>(m_SwapchainExtent.width);
	viewport.height   = static_cast<float>(m_SwapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_SwapchainExtent;
	vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);

	vkCmdDraw(m_CommandBuffer, 3, 1, 0, 0);

	// end render pass
	vkCmdEndRenderPass(m_CommandBuffer);

	if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to record command buffer!");
}