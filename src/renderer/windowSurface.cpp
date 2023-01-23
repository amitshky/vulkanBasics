#include "windowSurface.h"

#include <stdexcept>


WindowSurface::WindowSurface(GLFWwindow* windowContext, VkInstance vulkanInstance)
	: m_WindowContext{windowContext}, 
	  m_VulkanInstance{vulkanInstance}
{
	CreateWindowSurface();
}

WindowSurface::~WindowSurface()
{
	vkDestroySurfaceKHR(m_VulkanInstance, m_WindowSurface, nullptr);
}

void WindowSurface::CreateWindowSurface()
{
	if (glfwCreateWindowSurface(m_VulkanInstance, m_WindowContext, nullptr, &m_WindowSurface) != VK_SUCCESS)
		throw std::runtime_error("Failed to create window surface!");
}