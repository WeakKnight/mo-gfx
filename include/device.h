#pragma once
#include <vulkan/vulkan.hpp>
#include "common.h"

class Surface;

class Device
{
public:
	Device(VkPhysicalDevice physicalDevice, std::shared_ptr<Surface> surface);
	~Device();

	VkDevice GetApiHandle() const
	{
		return m_apiHandle;
	}

private:

	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

	VkQueue m_graphicsQueue;
	VkQueue m_transferQueue;
	VkQueue m_computeQueue;
	VkQueue m_presentQueue;

	uint32_t m_graphicsFamilyIndex = {};
	uint32_t m_computeFamilyIndex = {};
	uint32_t m_presentFamilyIndex = {};
	uint32_t m_transferFamilyIndex = {};

	VkDevice m_apiHandle = VK_NULL_HANDLE;
	std::shared_ptr<Surface> m_surface;
};