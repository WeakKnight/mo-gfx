#pragma once
#include <vulkan/vulkan.hpp>
#include <memory>

class Device;

class Image
{
public:
	Image(std::shared_ptr<Device> device, VkImage vkImage);
	~Image();

	VkImage GetApiHandle() const
	{
		return m_apiHandle;
	}

private:
	bool hadOwnerShip = true;

	std::shared_ptr<Device> m_device;
	VkImage m_apiHandle = VK_NULL_HANDLE;
};