#pragma once

#include "common.h"
#include <vulkan/vulkan.hpp>

class Instance
{
public:
	Instance(std::vector<std::string> enabledExtensions, std::vector<std::string> enabledLayers);
	~Instance();

	VkInstance GetApiHandle() const
	{
		return m_instance;
	}

	std::vector<VkExtensionProperties> GetExtensions() const 
	{
		return m_extensions;
	}

	VkPhysicalDevice GetPhysicalDevice() const 
	{
		return m_physicalDevice;
	}

private:

	bool CheckLayers(const std::vector<std::string> requestLayers) const;
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device) const;

	VkInstance m_instance = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

	std::vector<VkExtensionProperties> m_extensions;
	std::vector<VkLayerProperties> m_availableLayers;
	std::vector<VkPhysicalDevice> m_physicalDevices;
};