#include "pipeline_layout.h"
#include "device.h"

PipelineLayout::PipelineLayout(std::shared_ptr<Device> device)
	:m_device(device)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	auto result = vkCreatePipelineLayout(m_device->GetApiHandle(), &pipelineLayoutInfo, nullptr, &m_apiHandle);
	assert(result == VK_SUCCESS);
}

PipelineLayout::~PipelineLayout()
{
	if (m_apiHandle != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(m_device->GetApiHandle(), m_apiHandle, nullptr);
	}
}