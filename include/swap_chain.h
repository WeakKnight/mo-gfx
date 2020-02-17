#pragma once
#include <vulkan/vulkan.hpp>
#include <memory>

class Device;
class Surface;

class SwapChain
{
public:
	SwapChain(std::shared_ptr<Device> device, std::shared_ptr<Surface> surface);
	~SwapChain();

	VkSwapchainKHR GetApiHandle() const
	{
		return m_apiHandle;
	}

private:

	void InitSupportInfo();
	void ChooseFormat();
	void ChoosePresentMode();
	void ComputeExtent();
	void ComputeImageCount();

	std::shared_ptr<Surface> m_surface;
	std::shared_ptr<Device> m_device;
	VkSwapchainKHR m_apiHandle = VK_NULL_HANDLE;

	VkSurfaceCapabilitiesKHR m_capabilities;
	std::vector<VkSurfaceFormatKHR> m_supportedFormats;
	std::vector<VkPresentModeKHR> m_supportedPresentModes;

	uint32_t m_imageCount;

	VkSurfaceFormatKHR m_format;
	VkPresentModeKHR m_presentMode;
	VkExtent2D m_extent;
};