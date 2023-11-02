#include "window.h"

#include <stdexcept>


Window::Window(const char* title, int32_t width, int32_t height)
	: m_Title{ title }
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	glfwMakeContextCurrent(m_Window);
}

Window::~Window()
{
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}
