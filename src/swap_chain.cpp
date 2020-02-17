#include "swap_chain.h"
#include "device.h"
#include "surface.h"

SwapChain::SwapChain(std::shared_ptr<Device> device, std::shared_ptr<Surface> surface)
{
	m_device = device;
	m_surface = surface;

	InitSupportInfo();
	ChooseFormat();
	ChoosePresentMode();
	ComputeExtent();
	ComputeImageCount();

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface->GetApiHandle();
	createInfo.minImageCount = m_imageCount;
	createInfo.imageFormat = m_format.format;
	createInfo.imageColorSpace = m_format.colorSpace;
	createInfo.imageExtent = m_extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { m_device->GetGraphicsQueueIndex(), m_device->GetPresentQueueIndex() };

	if (m_device->GetGraphicsQueueIndex() != m_device->GetPresentQueueIndex())
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}


	createInfo.preTransform = m_capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = m_presentMode;
	createInfo.clipped = VK_TRUE;

	VkResult result = vkCreateSwapchainKHR(m_device->GetApiHandle(), &createInfo, nullptr, &m_apiHandle);
	assert(result == VK_SUCCESS);
}

SwapChain::~SwapChain()
{
	if (m_apiHandle != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_device->GetApiHandle(), m_apiHandle, nullptr);
	}
}

void SwapChain::InitSupportInfo()
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device->GetPhysicalDevice(), m_surface->GetApiHandle(), &m_capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_device->GetPhysicalDevice(), m_surface->GetApiHandle(), &formatCount, nullptr);

	if (formatCount != 0)
	{
		m_supportedFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_device->GetPhysicalDevice(), m_surface->GetApiHandle(), &formatCount, m_supportedFormats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_device->GetPhysicalDevice(), m_surface->GetApiHandle(), &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		m_supportedPresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_device->GetPhysicalDevice(), m_surface->GetApiHandle(), &presentModeCount, m_supportedPresentModes.data());
	}

	assert(formatCount != 0 && presentModeCount != 0);
}

void SwapChain::ChooseFormat()
{
	for (auto supportedFormat : m_supportedFormats)
	{
		if (supportedFormat.format == VK_FORMAT_B8G8R8A8_SRGB && supportedFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			m_format = supportedFormat;
			return;
		}
	}

	m_format = m_supportedFormats[0];
}

void SwapChain::ChoosePresentMode()
{
	m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
}

void SwapChain::ComputeExtent()
{
	m_extent = m_capabilities.currentExtent;
}

void SwapChain::ComputeImageCount()
{
	m_imageCount = m_capabilities.minImageCount + 1;
	if (m_imageCount > m_capabilities.maxImageCount)
	{
		m_imageCount = m_capabilities.maxImageCount;
	}

	assert(m_imageCount != 0);
}