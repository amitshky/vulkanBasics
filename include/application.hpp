#pragma once

#include <string>
#include <vector>
#include <optional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


// Validation layers settings
#ifdef NDEBUG // Release mode
	const bool enableValidationLayers = false;
#else         // Debug mode
	const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pDebugMessenger
);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);


struct QueueFamilyIndices
{
	// std::optional contains no value until you assign something to it
	// we can check if it contains a value by calling has_value()
	std::optional<uint32_t> graphicsFamily;

	inline bool IsComplete() const { return graphicsFamily.has_value(); }
};


class Application
{
public:
	Application(const std::string& title, int32_t width, int32_t height);
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void Run();

private:
	void InitWindow();
	void InitVulkan();
	void Cleanup();
	void CreateVulkanInstance();
	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();  // returns a required list of extensions based on whether validation layers are enabled or not
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( // `VKAPI_ATTR` and `VKAPI_CALL` ensures the right signature for Vulkan 
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageType, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbakck,
		void* pUserData
	);

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo);
	void SetupDebugMessenger();

	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice physicalDevice);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice);

	void CreateLogicalDevice();
	void CreateWindowSurface();

private:
	std::string m_Title;
	int32_t m_Width;
	int32_t m_Height;
	GLFWwindow* m_Window;
	VkInstance m_VulkanInstance;
	VkDebugUtilsMessengerEXT m_DebugMessenger; // handle for DebugCallback
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device; // logical device
	VkQueue m_GraphicsQueue; // handle to the queue
	VkSurfaceKHR m_WindowSurface;
};