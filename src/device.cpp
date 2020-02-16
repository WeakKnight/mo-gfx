#include "device.h"
#include "surface.h"
#include <set>

Device::Device(VkPhysicalDevice physicalDevice, std::shared_ptr<Surface> surface, std::vector<const char*> requiredExtensions)
{
	m_physicalDevice = physicalDevice;
	m_surface = surface;
	m_requiredExtensions = requiredExtensions;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (int i = 0; i < queueFamilies.size(); i++)
	{
		const VkQueueFamilyProperties& queueFamily = queueFamilies[i];
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
		{
			m_graphicsFamilyIndex = i;
		}

		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			m_computeFamilyIndex = i;
		}

		if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			m_transferFamilyIndex = i;
		}

		VkBool32 supportPresent = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface->GetApiHandle(), &supportPresent);
		if (supportPresent)
		{
			m_presentFamilyIndex = i;
		}
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { m_graphicsFamilyIndex, m_computeFamilyIndex, m_transferFamilyIndex, m_presentFamilyIndex };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) 
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	assert(CheckExtensionSupport(), true);

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = m_requiredExtensions.size();
	createInfo.ppEnabledExtensionNames = m_requiredExtensions.data();
	createInfo.enabledLayerCount = 0;

	VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &m_apiHandle);
	assert(result == VK_SUCCESS);

	vkGetDeviceQueue(m_apiHandle, m_graphicsFamilyIndex, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_apiHandle, m_computeFamilyIndex, 0, &m_computeQueue);
	vkGetDeviceQueue(m_apiHandle, m_presentFamilyIndex, 0, &m_presentQueue);
	vkGetDeviceQueue(m_apiHandle, m_transferFamilyIndex, 0, &m_transferQueue);
}

Device::~Device()
{
	if (m_apiHandle != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_apiHandle, nullptr);
	}
}

bool Device::CheckExtensionSupport() const
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	bool result = true;

	for (auto requiredExtension : m_requiredExtensions)
	{
		bool hadThis = false;
		for (auto availableExtension : availableExtensions)
		{
			if (strcmp(availableExtension.extensionName, requiredExtension))
			{
				hadThis = true;
			}
		}

		if (!hadThis)
		{
			result = false;
			break;
		}
	}

	return result;
}