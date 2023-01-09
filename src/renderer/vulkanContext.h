#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan.h>


// Validation layers settings
#ifdef NDEBUG // Release mode
	const bool enableValidationLayers = false;
#else         // Debug mode
	const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

class VulkanContext
{
public:
	VulkanContext(const std::string& title);
	~VulkanContext();

	inline VkInstance GetInstance() const { return m_VulkanInstance; }

private:
	void CreateInstance(const char* title);
	void SetupDebugMessenger();

	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();
	
	// `VKAPI_ATTR` and `VKAPI_CALL` ensures the right signature for Vulkan 
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbakck,void* pUserData);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo);

	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

private:
	VkInstance m_VulkanInstance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
};