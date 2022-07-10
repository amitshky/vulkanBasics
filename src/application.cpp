#include "application.h"
#include "vulkan/vulkan_core.h"

#include <stdexcept>
#include <cstdint>
#include <vector>
#include <iostream>

Application::Application(const std::string& title, int32_t width, int32_t height)
	: m_Title(title), m_Width(width), m_Height(height)
{
	InitWindow();
	InitVulkan();
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
}

void Application::Cleanup()
{
	vkDestroyInstance(m_VulkanInstance, nullptr);

	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Application::CreateVulkanInstance()
{
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
	// we need an extension to interface with the window
	uint32_t extensionCount = 0;
	const char** extensions = nullptr;
	extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = extensions;
	// specify global validation layers
	createInfo.enabledLayerCount = 0;

	// to check the available extensions
	// we dont need this for now
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions{extensionCount};
	// query extension details
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
	std::cout << "Available extensions (" << extensionCount << "):\n";
	for (auto& extension : availableExtensions)
		std::cout << "    " << extension.extensionName << '\n';


	// create an instance
	if (vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan Instance!");
}