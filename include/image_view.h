#pragma once
#include <vulkan/vulkan.hpp>
#include <memory>

class Device;
class Image;

class ImageView
{
public:
	ImageView(std::shared_ptr<Device> device, std::shared_ptr<Image> image, VkFormat format, VkImageViewType imageViewType);
	~ImageView();

	VkImageView GetApiHandle() const
	{
		return m_apiHandle;
	}

private:
	std::shared_ptr<Device> m_device;
	VkImageView m_apiHandle = VK_NULL_HANDLE;
	std::shared_ptr<Image> m_image;
	VkFormat m_format;
	VkImageViewType m_type;
};