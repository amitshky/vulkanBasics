#pragma once

#include <vulkan/vulkan.h>

#include <vector>


struct VulkanConfig
{
public:
	bool enableValidationLayers;
	int MAX_FRAMES_IN_FLIGHT;
	
	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;

	VulkanConfig() = default;

	VulkanConfig(bool enableValLayers, int maxFrames, const std::vector<const char*>& valLayers, const std::vector<const char*>& devExt)
		: enableValidationLayers{enableValLayers}, 
		  MAX_FRAMES_IN_FLIGHT{maxFrames},
		  validationLayers{valLayers},
		  deviceExtensions{devExt} 
	{

	}
};
