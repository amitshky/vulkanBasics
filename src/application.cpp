#include "application.hpp"

#include <stdexcept>
#include <cstdint>
#include <vector>
#include <iostream>


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
	PickPhysicalDevice();
}

void Application::Cleanup()
{
	if (enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);

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
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_Title.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// specify which extensions and validation layers to use
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	// we need extensions to interface with the window
	uint32_t extensionCount = 0;
	auto extensions = GetRequiredExtensions();  // returns a required list of extensions based on whether validation layers are enabled or not
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	// debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	// specify global validation layers
	if (enableValidationLayers) // include validation layers if enabled
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugMessengerInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessengerInfo;
	} 
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
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
	if (vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS)
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

	return indicies.IsComplete();
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
		
		if (indices.IsComplete())
			break;
	}

	return indices;
}