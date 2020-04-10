#pragma once
#include "gfx.h"
#include "string_utils.h"

class PipelineObject
{
public:
	static PipelineObject* Create()
	{
		PipelineObject* mat = new PipelineObject();
		return mat;
	}

	void Build(GFX::RenderPass renderPass, uint32_t subpassIndex, uint32_t subpassColorAttachmentCount, GFX::VertexBindings& vertexBindings, GFX::UniformBindings& uniformBindings, const std::string& vertShaderPath, const std::string& fragShaderPath, bool enableDepth, GFX::CullFace cullface = GFX::CullFace::Back);

	static void Destroy(PipelineObject* mat)
	{
		GFX::DestroyShader(mat->vertShader);
		GFX::DestroyShader(mat->fragShader);

		GFX::DestroyPipeline(mat->pipeline);
	}

	GFX::Shader vertShader;
	GFX::Shader fragShader;
	GFX::Pipeline pipeline;
};