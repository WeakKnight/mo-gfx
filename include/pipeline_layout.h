#pragma once
#include <vulkan/vulkan.hpp>
#include <memory>

class Device;

class PipelineLayout
{
public:

	PipelineLayout(std::shared_ptr<Device> device);
	~PipelineLayout();

	VkPipelineLayout GetApiHandle() const
	{
		return m_apiHandle;
	}

private:
	VkPipelineLayout m_apiHandle = VK_NULL_HANDLE;
	std::shared_ptr<Device> m_device;
};