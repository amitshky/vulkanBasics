#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
public:
	Window(const std::string& title, int32_t width, int32_t height);
	~Window();

	inline const char* GetTitle() const { return m_Title.c_str(); }
	inline GLFWwindow* GetWindowContext() const { return m_Window; }

private:
	std::string m_Title;
	GLFWwindow* m_Window;
	bool m_FramebufferResized = false;
};
