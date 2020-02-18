#include "render_pass.h"
#include "device.h"

RenderPass::RenderPass(std::shared_ptr<Device> device)
	:m_device(device)
{

}

RenderPass::~RenderPass()
{
	if (m_apiHandle != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(m_device->GetApiHandle(), m_apiHandle, VK_NULL_HANDLE);
	}
}