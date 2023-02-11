add_executable(
	${PROJECT_NAME}

	main.cpp
	core/application.cpp
	core/window.cpp
	
	renderer/vulkanContext.cpp
	renderer/windowSurface.cpp
	renderer/device.cpp
	renderer/swapchain.cpp
	renderer/shader.cpp
	renderer/pipeline.cpp
	renderer/texture.cpp

	renderer/buffer/commandBuffer.cpp
	renderer/buffer/vertexBuffer.cpp
	renderer/buffer/indexBuffer.cpp
	renderer/buffer/uniformBuffer.cpp
	
	renderer/camera.cpp

	utils/utils.cpp
	utils/commandBufferUtils.cpp
	utils/bufferUtils.cpp
	utils/imageUtils.cpp
)