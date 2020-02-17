#include "image_view.h"
#include "image.h"
#include "device.h"

ImageView::ImageView(std::shared_ptr<Device> device, std::shared_ptr<Image> image, VkFormat format, VkImageViewType imageViewType)
{
	m_device = device;
	m_image = image;
	m_format = format;
	m_type = imageViewType;

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image->GetApiHandle();
	createInfo.format = m_format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	createInfo.viewType = m_type;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkResult result = vkCreateImageView(m_device->GetApiHandle(), &createInfo, nullptr, &m_apiHandle);
	assert(result == VK_SUCCESS);
}

ImageView::~ImageView()
{
	if (m_apiHandle != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_device->GetApiHandle(), m_apiHandle, nullptr);
	}
}