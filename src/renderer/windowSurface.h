#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


class WindowSurface
{
public:
	WindowSurface(GLFWwindow* windowContext, VkInstance vulkanInstance);
	~WindowSurface();

	inline VkSurfaceKHR GetSurface() const { return m_WindowSurface; }

private:
	void CreateWindowSurface();

private:
	GLFWwindow* m_WindowContext;
	VkInstance m_VulkanInstance;

	VkSurfaceKHR m_WindowSurface;
};