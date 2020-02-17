#include "image.h"
#include "device.h"

Image::Image(std::shared_ptr<Device> device, VkImage vkImage)
{
	m_device = device;
	m_apiHandle = vkImage;

	hadOwnerShip = false;
}

Image::~Image()
{
	if (hadOwnerShip)
	{
		vkDestroyImage(m_device->GetApiHandle(), m_apiHandle, nullptr);
	}
}