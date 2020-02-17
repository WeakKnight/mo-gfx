#pragma once
#include <vulkan/vulkan.hpp>
#include <string>
#include <memory>
#include "shaderc/shaderc.hpp"

class Device;

class ShaderModule
{
public:
	ShaderModule(std::shared_ptr<Device> device, const std::string& path, shaderc_shader_kind kind);
	~ShaderModule();

	VkShaderModule GetApiHandle() const
	{
		return m_apiHandle;
	}

	std::vector<uint32_t> CompileFile(const std::string& sourceName,
		shaderc_shader_kind kind,
		const std::string& source,
		bool optimize) const;

private:
	std::shared_ptr<Device> m_device;
	VkShaderModule m_apiHandle = VK_NULL_HANDLE;
};