#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


class Application
{
public:
	Application(const std::string& title, int32_t width, int32_t height);

	void Run();

private:
	void InitWindow();
	void InitVulkan();
	void Cleanup();

private:
	std::string m_Title;
	int32_t m_Width;
	int32_t m_Height;
	GLFWwindow* m_Window;
};