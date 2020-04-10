#pragma once
#include <gfx.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "string_utils.h"

class ShadowMap
{
public:
	~ShadowMap()
	{
		GFX::DestroyPipeline(pipeline);
		GFX::DestroyShader(vertShader);
		GFX::DestroyShader(fragShader);
		GFX::DestroyRenderPass(renderPass);
		GFX::DestroyUniformLayout(uniformLayout);
		GFX::DestroyUniform(uniform);
		GFX::DestroyBuffer(uniformBuffer);
	}

	class ShadowMapUniformObject
	{
	public:
		glm::mat4 view;
		glm::mat4 proj;
	};

	static ShadowMap* Create()
	{
		auto result = new ShadowMap();

		result->CreateShader();
		result->CreateRenderPass();

		return result;
	}

	GFX::Buffer uniformBuffer = {};
	GFX::UniformLayout uniformLayout = {};
	GFX::Uniform uniform = {};
	GFX::Pipeline pipeline = {};
	GFX::RenderPass renderPass = {};
	GFX::Shader vertShader = {};
	GFX::Shader fragShader = {};

	uint32_t mapSize = 1024;

private:
	void CreateShader()
	{
		GFX::ShaderDescription vertShaderDesc = {};
		vertShaderDesc.codes = StringUtils::ReadFile("screen-space-reflection/shadow.vert");
		vertShaderDesc.stage = GFX::ShaderStage::Vertex;
		vertShaderDesc.name = "screen-space-reflection/shadow.vert";

		vertShader = GFX::CreateShader(vertShaderDesc);

		GFX::ShaderDescription fragShaderDesc = {};
		fragShaderDesc.codes = StringUtils::ReadFile("screen-space-reflection/shadow.frag");
		fragShaderDesc.stage = GFX::ShaderStage::Fragment;
		fragShaderDesc.name = "screen-space-reflection/shadow.frag";

		fragShader = GFX::CreateShader(fragShaderDesc);
	}

	void CreateRenderPass()
	{
		GFX::RenderPassDescription renderPassDesc = {};

		GFX::AttachmentDescription depthAttachmentDescription = {};
		depthAttachmentDescription.format = GFX::Format::DEPTH_24UNORM_STENCIL_8INT;
		depthAttachmentDescription.width = mapSize;
		depthAttachmentDescription.height = mapSize;
		depthAttachmentDescription.loadAction = GFX::AttachmentLoadAction::Clear;
		depthAttachmentDescription.storeAction = GFX::AttachmentStoreAction::Store;
		depthAttachmentDescription.type = GFX::AttachmentType::DepthStencil;
		depthAttachmentDescription.clearValue.SetDepth(1.0f);
		
		renderPassDesc.attachments.push_back(depthAttachmentDescription);
		renderPassDesc.width = mapSize;
		renderPassDesc.height = mapSize;
	}

	ShadowMap()
	{
	}
};