#pragma once
#include <vulkan/vulkan.hpp>
#include <memory>

class Device;

class SwapChain
{
public:
	SwapChain(std::shared_ptr<Device> device);
private:

	std::shared_ptr<Device> m_device;
	VkSwapchainKHR m_apiHandle = VK_NULL_HANDLE;

	VkSurfaceCapabilitiesKHR m_capabilities;
	std::vector<VkSurfaceFormatKHR> m_supportedFormats;
	std::vector<VkPresentModeKHR> m_supportedPresentModes;
};