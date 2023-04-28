#include "pipeline.h"

#include <stdexcept>

#include "shader.h"
#include "renderer/buffer/vertexBuffer.h"


Pipeline::Pipeline(VkDevice deviceVk, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples)
	: m_DeviceVk{deviceVk},
	  m_RenderPass{renderPass},
	  m_MsaaSamples{msaaSamples}
{
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
}

Pipeline::~Pipeline()
{
	vkDestroyDescriptorSetLayout(m_DeviceVk, m_DescriptorSetLayout, nullptr);

	vkDestroyPipeline(m_DeviceVk, m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_DeviceVk, m_PipelineLayout, nullptr);
}

void Pipeline::CreateGraphicsPipeline()
{
	// shaders
	Shader m_VertexShader{"assets/shaders/gradientTriangle.vert.spv", ShaderType::VERTEX, m_DeviceVk};
	Shader m_FragmentShader{"assets/shaders/gradientTriangle.frag.spv", ShaderType::FRAGMENT, m_DeviceVk};

	VkPipelineShaderStageCreateInfo shaderStages[] = { m_VertexShader.GetShaderStage(), m_FragmentShader.GetShaderStage() };

	// fixed functions
	// vertex input
	auto bindingDescription    = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
	vertexInputCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount   = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions      = &bindingDescription;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	// viewport state
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount  = 1;

	// rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
	rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;                // if true, clamp to depth instead of discarding
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;                // if true, the geometry never passes through rasterizer stage
	rasterizationStateCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;    // how fragments are generated
	rasterizationStateCreateInfo.lineWidth               = 1.0f;                    // thickness of lines
	rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_NONE;   // type of face culling; cull the back face
	// we specify counter clockwise because in the projection matrix we flipped the y-coord
	rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_CLOCKWISE; // vertex order for faces to be considered front-face
	// the depth value can be altered by adding a constant value based on fragment slope
	rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp          = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor    = 0.0f;

	// multisampling
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
	multisampleStateCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable   = VK_FALSE;
	multisampleStateCreateInfo.rasterizationSamples  = m_MsaaSamples;
	multisampleStateCreateInfo.minSampleShading      = 1.0f;
	multisampleStateCreateInfo.pSampleMask           = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable      = VK_FALSE;

	// depth and stencil testing
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};

	// color blending
	// configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable         = VK_FALSE;
	//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	//colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	//colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	//colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	// the above config does the following
	// finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
	// finalColor.a = newAlpha.a;

	// configuration for global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
	colorBlendStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable     = VK_FALSE;
	colorBlendStateCreateInfo.logicOp           = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount   = 1;
	colorBlendStateCreateInfo.pAttachments      = &colorBlendAttachment;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

	// dynamic states
	// allows you to specify the data at drawing time
	std::vector<VkDynamicState> dynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates    = dynamicStates.data();

	// pipeline layout
	// specify uniforms
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount         = 1;
	pipelineLayoutCreateInfo.pSetLayouts            = &m_DescriptorSetLayout;
	// push constants are another way of passing dynamic values to the shaders
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;

	if (vkCreatePipelineLayout(m_DeviceVk, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline!");


	// graphics pipeline
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// config all previos objects
	graphicsPipelineCreateInfo.stageCount          = 2;
	graphicsPipelineCreateInfo.pStages             = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState   = &vertexInputCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState   = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState  = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState    = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState       = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout              = m_PipelineLayout;
	graphicsPipelineCreateInfo.renderPass          = m_RenderPass;
	graphicsPipelineCreateInfo.subpass             = 0;
	// pipeline can be created by deriving from a previous pipeline
	// less expensive to set up a pipeline if it has common functionality with an existing pipeline
	graphicsPipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex   = -1;

	// multiple graphicsPipelineCreateInfo can be passed
	// pipelineCache (2nd param) can be used to store and reuse data
	if (vkCreateGraphicsPipelines(m_DeviceVk, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline!");
}

void Pipeline::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding            = 0; // binding used in the shader
	uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount    = 1;
	uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT; // shader stage that the descriptor will be referencing
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo{};
	descriptorLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorLayoutCreateInfo.pBindings    = bindings.data();

	if (vkCreateDescriptorSetLayout(m_DeviceVk, &descriptorLayoutCreateInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");
}