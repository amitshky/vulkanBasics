#include "shader.h"

#include <fstream>


Shader::Shader(const std::string& path, ShaderType type, VkDevice device)
	: m_Path{path}, 
	  m_Type{type},
	  m_Device{device},
	  m_ShaderCode{},
	  m_ShaderModule{VK_NULL_HANDLE},
	  m_ShaderStage{} // has to be default initialized to initialize all its fields
{
	LoadShader();
	CreateShaderModule();
	CreateShaderStage();
}

Shader::~Shader()
{
	vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
}

void Shader::LoadShader()
{
	std::ifstream file{ m_Path, std::ios::binary | std::ios::ate };
	if (!file.is_open())
		throw std::runtime_error("Error opening shader file: " + m_Path);

	const size_t fileSize = static_cast<size_t>(file.tellg());
	m_ShaderCode.resize(fileSize);

	file.seekg(0);
	file.read(m_ShaderCode.data(), fileSize);
	file.close();
}

void Shader::CreateShaderModule()
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = m_ShaderCode.size();
	// code is in char but shaderModule expects it to be in uint32_t
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(m_ShaderCode.data());

	if (vkCreateShaderModule(m_Device, &shaderModuleCreateInfo, nullptr, &m_ShaderModule) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module!");
}

void Shader::CreateShaderStage()
{
	m_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	
	if (m_Type == ShaderType::VERTEX)
		m_ShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	else if (m_Type == ShaderType::FRAGMENT)
		m_ShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	
	m_ShaderStage.module = m_ShaderModule;
	m_ShaderStage.pName  = "main"; // entrypoint // so it is possible to combine multiple shaders into a single module
}