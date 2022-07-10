#include <exception>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "application.h"


int main()
{
	Application* app = new Application("Vulkan basics", 800, 600);

	try
	{
		app->Run();
	}
	catch(const std::exception& e)
	{
		std::cout << e.what() << '\n';
		return EXIT_FAILURE;
	}

	delete app;
}