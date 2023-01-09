#include "vulkanContext.h"

#include <stdexcept>
#include <iostream>

#include <GLFW/glfw3.h>


VulkanContext::VulkanContext(const std::string& title)
{
	CreateInstance(title.c_str());
	SetupDebugMessenger();
}

VulkanContext::~VulkanContext()
{ 
	if (enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);

	vkDestroyInstance(m_VulkanInstance, nullptr);
}

void VulkanContext::CreateInstance(const char* title)
{
	if (enableValidationLayers && !CheckValidationLayerSupport())
		throw std::runtime_error("Validation layers requested, but not available!");

	// info about our application
	VkApplicationInfo appInfo{};
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = title;
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
	instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	// specify global validation layers
	if (enableValidationLayers) // include validation layers if enabled
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

		// debug messenger
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
		PopulateDebugMessengerCreateInfo(debugMessengerInfo);
		instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessengerInfo;
	} 
	else
	{
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.pNext = nullptr;
	}

	// to check the available extensions
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

void VulkanContext::SetupDebugMessenger()
{
	if (!enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	PopulateDebugMessengerCreateInfo(debugMessengerInfo);

	if (CreateDebugUtilsMessengerEXT(m_VulkanInstance, &debugMessengerInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
		throw std::runtime_error("Failed to setup debug messenger!");
}

bool VulkanContext::CheckValidationLayerSupport()
{
	// check if all of the requested layers are available
	uint32_t validationLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);
	std::vector<VkLayerProperties> availableLayerProperites{validationLayerCount};
	vkEnumerateInstanceLayerProperties(&validationLayerCount, availableLayerProperites.data());

	for (const auto& layer : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayerProperites)
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

std::vector<const char*> VulkanContext::GetRequiredExtensions()
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
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,    // to check the severity of the message
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, // contains the actual details of the message
	void* pUserData                                            // allows you to pass your own data
)
{
	std::cerr << "Validation layer: " << pCallbackData->pMessage << '\n';
	return VK_FALSE;
}

void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo) 
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

VkResult VulkanContext::CreateDebugUtilsMessengerEXT(VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pDebugMessenger
)
{
	// requires a valid instance to have been created
	// so we cannot debug any issues in vkCreateInstance
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"); // returns nullptr if the function couldn't be loaded
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanContext::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	// must be destroyed before instance is destroyed
	// so we cannot debug any issues in vkDestroyInstance
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}
