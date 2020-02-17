#pragma once
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>

class Device;
class ShaderModule;
class SwapChain;

class GraphicsPipeline
{
public:
	GraphicsPipeline(std::shared_ptr<Device> device, std::shared_ptr<SwapChain> swapChain, const std::string& vertPath, const std::string& fragPath);
	~GraphicsPipeline();

	VkPipeline GetApiHandle() const
	{
		return m_apiHandle;
	}

private:


	std::shared_ptr<ShaderModule> m_vertShaderModule;
	std::shared_ptr<ShaderModule> m_fragShaderModule;

	std::shared_ptr<SwapChain> m_swapChain;
	std::shared_ptr<Device> m_device;
	VkPipeline m_apiHandle;
};