#include "swapchain.h"

#include <array>
#include <algorithm>
#include <stdexcept>

#include "utils/utils.h"
#include "utils/imageUtils.h"


Swapchain::Swapchain(GLFWwindow* windowContext, const Device* device, VkSurfaceKHR windowSurface)
	: m_WindowContext{windowContext},
	  m_Device{device},
	  m_WindowSurface{windowSurface}
{
	CreateSwapchain();
	CreateImageViews();
	CreateRenderPass();
	CreateDepthResources();
	CreateFramebuffers();
}

Swapchain::~Swapchain()
{
	vkDestroyRenderPass(m_Device->GetDevice(), m_RenderPass, nullptr);
	CleanupSwapchain();
}

void Swapchain::CreateSwapchain()
{
	SwapchainSupportDetails swapchainSupport = utils::QuerySwapchainSupport(m_Device->GetPhysicalDevice(), m_WindowSurface);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR   presentMode   = ChooseSwapPresentMode(swapchainSupport.presentModes);
	VkExtent2D         extent        = ChooseSwapExtent(swapchainSupport.capabilities);

	// specify how many images we want in the swapchain
	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
	if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
		imageCount = swapchainSupport.capabilities.maxImageCount;

	// create swapchain
	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = m_WindowSurface;

	// swapchain image details
	swapchainCreateInfo.minImageCount    = imageCount;
	swapchainCreateInfo.imageFormat      = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent      = extent;
	swapchainCreateInfo.imageArrayLayers = 1; // number of layers in each image
	swapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // what kind of operation the images in the swap chain be used for

	// handle swapchain images across multiple queue families
	QueueFamilyIndices indices = utils::FindQueueFamilies(m_Device->GetPhysicalDevice(), m_WindowSurface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;  // image used across multiple queue families
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;  //image owned by one queue family at a time
	}

	swapchainCreateInfo.preTransform   = swapchainSupport.capabilities.currentTransform;  // we can rotate, flip, etc // we can specify that certain transformation can be applied
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;               // specify if alpha channel is used for blending
	swapchainCreateInfo.presentMode    = presentMode;
	swapchainCreateInfo.clipped        = VK_TRUE;
	swapchainCreateInfo.oldSwapchain   = VK_NULL_HANDLE;                                  // if new swapchain is to be created, the old one should be referenced here

	if (vkCreateSwapchainKHR(m_Device->GetDevice(), &swapchainCreateInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Swapchain!");

	// get swapchain images
	vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &imageCount, nullptr);
	m_SwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &imageCount, m_SwapchainImages.data());

	m_SwapchainImageFormat = surfaceFormat.format;
	m_SwapchainExtent = extent;
}

void Swapchain::CreateImageViews()
{
	m_SwapchainImageViews.resize(m_SwapchainImages.size());

	for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
		m_SwapchainImageViews[i] = utils::img::CreateImageView(m_Device->GetDevice(), m_SwapchainImages[i], m_SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Swapchain::CreateRenderPass()
{
	// color attachment
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format  = m_SwapchainImageFormat; // the format should match the format of the swapchain
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;  // we dont have multisampling yet
	// determining what to do with the data in the attachment
	// for color and depth data
	colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;  // clear the framebuffer before drawing the next frame
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store the rendered contents in the memory
	// for stencil data
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// the textures and framebuffer are represented by `VkImage`, with a certain pixel format
	// the layout of the pixel format can be changed based on what you're trying to do
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;       // we don't care about the previous layout of the image (before the render pass begins)
	colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // the image to be presented in the swapchain (when the render pass finishes)
	
	// attachment references
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; // index of attachment
	colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// depth attachment
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format         = FindDepthFormat();
	depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE; // we don't care about storing depth data
	depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttacmentRef{};
	depthAttacmentRef.attachment = 1;
	depthAttacmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	// subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS; // graphics subpass
	subpass.colorAttachmentCount = 1;
	// the index of the attachment in this array is directly referenced from the fragment shader // layout (location=0) out vec4 outColor
	subpass.pColorAttachments       = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttacmentRef;

	// subpass dependencies control the image layout transitions
	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass    = VK_SUBPASS_EXTERNAL; // implicit subpass before and after the render pass
	subpassDependency.dstSubpass    = 0; // our subpass
	// specify the operation to wait on
	subpassDependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment }; 
	// render pass
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCreateInfo.pAttachments    = attachments.data();
	renderPassCreateInfo.subpassCount    = 1;
	renderPassCreateInfo.pSubpasses      = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies   = &subpassDependency;

	if (vkCreateRenderPass(m_Device->GetDevice(), &renderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass!");
}

void Swapchain::CreateDepthResources()
{
	VkFormat depthFormat = FindDepthFormat();

	utils::img::CreateImage(m_Device->GetDevice(), m_Device->GetPhysicalDevice(), m_SwapchainExtent.width, m_SwapchainExtent.height, depthFormat, 
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
	m_DepthImageView = utils::img::CreateImageView(m_Device->GetDevice(), m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	// we don't need to map or copy another image to it, because it is going to be cleared at the start of the render.
}

void Swapchain::CreateFramebuffers()
{
	m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());

	for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i)
	{
		std::array<VkImageView, 2> attachments = {
			m_SwapchainImageViews[i],
			m_DepthImageView
		};

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass      = m_RenderPass;
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferCreateInfo.pAttachments    = attachments.data();
		framebufferCreateInfo.width           = m_SwapchainExtent.width;
		framebufferCreateInfo.height          = m_SwapchainExtent.height;
		framebufferCreateInfo.layers          = 1;

		if (vkCreateFramebuffer(m_Device->GetDevice(), &framebufferCreateInfo, nullptr, &m_SwapchainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create framebuffer!");
	}
}

void Swapchain::CleanupSwapchain()
{
	vkDestroyImageView(m_Device->GetDevice(), m_DepthImageView, nullptr);
	vkDestroyImage(m_Device->GetDevice(), m_DepthImage, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_DepthImageMemory, nullptr);

	for (auto framebuffer : m_SwapchainFramebuffers)
		vkDestroyFramebuffer(m_Device->GetDevice(), framebuffer, nullptr);

	for (const auto& imageView : m_SwapchainImageViews)
		vkDestroyImageView(m_Device->GetDevice(), imageView, nullptr);

	vkDestroySwapchainKHR(m_Device->GetDevice(), m_Swapchain, nullptr);
}

void Swapchain::RecreateSwapchain()
{
	// window minimized
	int width  = 0;
	int height = 0;

	glfwGetFramebufferSize(m_WindowContext, &width, &height);
	while (width == 0 || height == 0)
	{
		// wait while window is minimized
		glfwGetFramebufferSize(m_WindowContext, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_Device->GetDevice());

	CleanupSwapchain(); // cleanup previous swapchain objects

	// we may have to recreate renderpasses as well if the swapchain's format changes

	CreateSwapchain();
	CreateImageViews();
	CreateDepthResources();
	CreateFramebuffers();
}

// swapchain helper functions
VkSurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	// VkSurfaceFormatKHR contains format and colorspace
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	// normally we need to rank the available formats
	return availableFormats[0];
}

VkPresentModeKHR Swapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes) 
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	// resolution of window and the swap chain image not equal
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else
	{
		int width = 0;
		int height = 0;
		// screen coordinates might not correspond to pixels in high DPI displays so we cannot just use the original width and height
		glfwGetFramebufferSize(m_WindowContext, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		// bind the width and height to allowed range
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.width = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	};
}


// depth resources helper functions
VkFormat Swapchain::FindSupportedFormat(const std::vector<VkFormat>& canditateFormats, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (auto format : canditateFormats)
	{
		VkFormatProperties prop;
		vkGetPhysicalDeviceFormatProperties(m_Device->GetPhysicalDevice(), format, &prop);

		if (tiling == VK_IMAGE_TILING_LINEAR && (prop.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (prop.optimalTilingFeatures & features) == features)
			return format;
	}
	throw std::runtime_error("Failed to find supported format!");
}

VkFormat Swapchain::FindDepthFormat() 
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, 
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool Swapchain::HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
