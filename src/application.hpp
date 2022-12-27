#pragma once

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "camera.h"


// Validation layers settings
#ifdef NDEBUG // Release mode
	const bool enableValidationLayers = false;
#else         // Debug mode
	const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

constexpr int MAX_FRAMES_IN_FLIGHT = 2;


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
	std::optional<uint32_t> presentFamily;

	inline bool IsComplete() 
	{ 
		return graphicsFamily.has_value() && presentFamily.has_value(); 
	}
};

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding   = 0; // specifies the index of the binding in the array of bindings
		bindingDescription.stride    = sizeof(Vertex); // specifies the number of bytes from one entry to the next
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // move to the next data entry after each vertex

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
	{
		// for position
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
		attributeDescriptions[0].binding  = 0; // index of the per-vertex data
		attributeDescriptions[0].location = 0; // references the location directive of the input in the vertex shader.
		attributeDescriptions[0].format   = VK_FORMAT_R32G32_SFLOAT; // type of data; position has 2 floats
		attributeDescriptions[0].offset   = offsetof(Vertex, pos); // number of bytes from the begining of the per-vertex data
		// for color
		attributeDescriptions[1].binding  = 0; // index of the per-vertex data
		attributeDescriptions[1].location = 1; // references the location directive of the input in the vertex shader.
		attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT; // type of data; color has 3 floats
		attributeDescriptions[1].offset   = offsetof(Vertex, color); // number of bytes from the begining of the per-vertex data

		return attributeDescriptions;
	}
};

struct UniformBufferObject 
{
	// explicitly speicify alignments
	// because vulkan expects structs to be in a specific alignment with the structs in the shaders
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
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

	bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void CreateSwapchain();
	void CreateImageViews();

	void CreateRenderPass();
	void CreateGraphicsPipeline();
	std::vector<char> LoadShader(const std::string& filepath);
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void CreateSyncObjects();
	void DrawFrame();

	void CleanupSwapchain();
	void RecreateSwapchain();
	
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer cmdBuff);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void CreateDescriptorSetLayout();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentFrameIdx);
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void ProcessInput();
	static void OnMouseMove(GLFWwindow* window, double xpos, double ypos);

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, 
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void CreateTextureImage();
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

private:
	std::string m_Title;
	int32_t m_Width;
	int32_t m_Height;
	GLFWwindow* m_Window;
	VkInstance m_VulkanInstance;
	VkDebugUtilsMessengerEXT m_DebugMessenger; // handle for DebugCallback

	// devices and queues
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device; // logical device
	VkQueue m_GraphicsQueue; // handle to graphics queue
	VkSurfaceKHR m_WindowSurface;
	VkQueue m_PresentQueue; // handle to presentation queue

	// swapchain
	VkSwapchainKHR m_Swapchain;
	std::vector<VkImage> m_SwapchainImages;
	VkFormat m_SwapchainImageFormat;
	VkExtent2D m_SwapchainExtent;
	std::vector<VkImageView> m_SwapchainImageviews;

	// render pass
	VkRenderPass m_RenderPass;

	// uniform buffer descriptor layout
	VkDescriptorSetLayout m_DescriptorSetLayout;
	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformBuffersMemory;
	std::vector<void*> m_UniformBuffersMapped;
	// descriptor pool
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	// pipeline
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;

	// framebuffer
	std::vector<VkFramebuffer> m_SwapchainFramebuffers;

	// command buffer
	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	// synchronization objects
	// semaphores to sync gpu operations and fence to sync cpu operation with the gpu operation
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;

	// frames in-flight
	uint32_t m_CurrentFrameIdx = 0;

	// check for resize
	bool m_FramebufferResized = false;

	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_VertexBufferMemory;
	VkBuffer m_IndexBuffer;
	VkDeviceMemory m_IndexBufferMemory;

	Camera m_Camera;

	// for delta time
	float m_LastFrameTime = 0.0f;
	float m_DeltaTime     = 0.0f;

	// textures
	VkImage m_TextureImage;
	VkDeviceMemory m_TextureImageMemory;
};
