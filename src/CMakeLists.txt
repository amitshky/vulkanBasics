add_executable(
	${PROJECT_NAME}

	main.cpp
	core/application.cpp
	core/window.cpp
	
	renderer/vulkanContext.cpp
	renderer/windowSurface.cpp
	renderer/device.cpp
	renderer/swapchain.cpp
	renderer/camera.cpp
)