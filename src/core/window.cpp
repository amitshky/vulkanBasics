#include "window.h"

Window::Window(const std::string& title, int32_t width, int32_t height)
	: m_Title{title}
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

Window::~Window()
{
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}
