#pragma once

#include <string>

#include <GLFW/glfw3.h>

class Window
{
public:
	Window(const char* title, int32_t width, int32_t height);
	~Window();

	inline GLFWwindow* GetWindowContext() const { return m_Window; }

private:
	const char* m_Title;
	GLFWwindow* m_Window;
};
