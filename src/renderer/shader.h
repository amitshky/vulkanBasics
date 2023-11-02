#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>


enum class ShaderType
{
	VERTEX,
	FRAGMENT
};

class Shader
{
public:
	Shader(const std::string& path, ShaderType type, VkDevice device);
	~Shader();

	inline VkPipelineShaderStageCreateInfo GetShaderStage() const { return m_ShaderStage; }

private:
	void LoadShader();
	void CreateShaderModule();
	void CreateShaderStage();

private:
	std::string m_Path;
	ShaderType m_Type;
	VkDevice m_DeviceVk;

	std::vector<char> m_ShaderCode;

	VkShaderModule m_ShaderModule;
	VkPipelineShaderStageCreateInfo m_ShaderStage;
};