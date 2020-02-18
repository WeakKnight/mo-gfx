#pragma once
#include <vulkan//vulkan.hpp>
#include <memory>

class Device;

class RenderPass
{
public:

	RenderPass(std::shared_ptr<Device> device);
	~RenderPass();


	VkRenderPass GetApiHandle() const
	{
		return m_apiHandle;
	}

private:
	std::shared_ptr<Device> m_device;
	VkRenderPass m_apiHandle = VK_NULL_HANDLE;
};