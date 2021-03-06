#include "material.h"

void PipelineObject::Build(GFX::RenderPass renderPass, uint32_t subpassIndex, uint32_t subpassColorAttachmentCount, GFX::VertexBindings& vertexBindings, GFX::UniformBindings& uniformBindings, const std::string& vertShaderPath, const std::string& fragShaderPath, bool enableDepth, GFX::CullFace cullface)
{
	// Shader Creation
	GFX::ShaderDescription vertDesc = {};
	vertDesc.name = vertShaderPath;
	vertDesc.codes = StringUtils::ReadFile(vertShaderPath);
	vertDesc.stage = GFX::ShaderStage::Vertex;

	vertShader = GFX::CreateShader(vertDesc);

	GFX::ShaderDescription fragDesc = {};
	fragDesc.name = fragShaderPath;
	fragDesc.codes = StringUtils::ReadFile(fragShaderPath);
	fragDesc.stage = GFX::ShaderStage::Fragment;

	fragShader = GFX::CreateShader(fragDesc);

	GFX::GraphicsPipelineDescription pipelineDesc = {};
	pipelineDesc.enableDepthTest = enableDepth;
	pipelineDesc.enableStencilTest = false;
	pipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
	pipelineDesc.renderPass = renderPass;
	pipelineDesc.subpass = subpassIndex;
	pipelineDesc.vertexBindings = vertexBindings;
	pipelineDesc.uniformBindings = uniformBindings;
	pipelineDesc.shaders.push_back(vertShader);
	pipelineDesc.shaders.push_back(fragShader);
	pipelineDesc.cullFace = cullface;

	for (int i = 0; i < subpassColorAttachmentCount; i++)
	{
		pipelineDesc.blendStates.push_back({});
	}

	pipeline = GFX::CreatePipeline(pipelineDesc);
}