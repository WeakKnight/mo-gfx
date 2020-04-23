#pragma once
#include <gfx.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <array>

#include "string_utils.h"
#include "mesh.h"

#include "camera.h"

#define SHADOW_MAP_CASCADE_COUNT 3
#define SHADOW_MAP_SIZE 4096

class ShadowMap
{
public:
	~ShadowMap()
	{
		GFX::DestroyPipeline(pipeline0);
		GFX::DestroyPipeline(pipeline1);
		GFX::DestroyPipeline(pipeline2);
		GFX::DestroyShader(vertShader);
		GFX::DestroyShader(fragShader);
		GFX::DestroyUniformLayout(uniformLayout);
		GFX::DestroyUniform(uniform0);
		GFX::DestroyBuffer(uniformBuffer0);
		GFX::DestroyUniform(uniform1);
		GFX::DestroyBuffer(uniformBuffer1);
		GFX::DestroyUniform(uniform2);
		GFX::DestroyBuffer(uniformBuffer2);
		GFX::DestroyRenderPass(renderPass);
	}

	struct Cascade
	{
		float splitDepth;
		glm::mat4 proj;
		glm::mat4 view;
	};

	std::array<Cascade, SHADOW_MAP_CASCADE_COUNT> cascades = {};
	float cascadeSplitLambda = 0.75f;
	float visualize = 0.0;
	float contactShadowVisualize = 0.0;
	float irradianceVisualize = 0.0;

	class ShadowMapUniformObject
	{
	public:
		glm::mat4 view;
		glm::mat4 proj;
		// split 0, split 1, split2, currentIndex
		glm::vec4 splitPoints;
		glm::vec4 config0;
		glm::vec4 nothing1;
		glm::vec4 nothing2;
	};

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	static ShadowMap* Create()
	{
		auto result = new ShadowMap();

		result->CreateRenderPass();
		result->CreateShader();
		result->CreatePipeline();

		return result;
	}

	// return proj matrix for shadow map
	void ComputeShadowMatrix(Camera* camera, glm::vec3 lightDir)
	{
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

		float nearClip = camera->near;
		float farClip = camera->far;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera furstum
		// Based on method presentd in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) 
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = cascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) 
		{
			float splitDist = cascadeSplits[i];
			float splitDepth = (camera->near + splitDist * clipRange) * -1.0f;

			float near = (camera->near + lastSplitDist * clipRange);
			float far = (camera->near + splitDist * clipRange);
			float VerticalFov = glm::radians(camera->fov);

			float yn = near * tanf(VerticalFov * 0.5f);
			float yf = far * tanf(VerticalFov * 0.5f);
			float xn = camera->aspect * yn;
			float xf = camera->aspect * yf;
			
			glm::mat4 utilLightViewMatrix = glm::lookAt(glm::vec3(0.0f) - (lightDir), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			glm::vec3 corners[8] =
			{
				glm::vec3(xn,yn,-near),
				glm::vec3(-xn,yn,-near),
				glm::vec3(xn,-yn,-near),
				glm::vec3(-xn,-yn,-near),
				glm::vec3(xf,yf,-far),
				glm::vec3(-xf,yf,-far),
				glm::vec3(xf,-yf,-far),
				glm::vec3(-xf,-yf,-far)
			};

			float minX = INFINITY;
			float maxX = -INFINITY;
			float minY = INFINITY;
			float maxY = -INFINITY;
			float minZ = INFINITY;
			float maxZ = -INFINITY;

			for (int j = 0; j < 8; j++) {

				// Transform the frustum coordinate from view to world space
				glm::vec4 vW = glm::inverse(camera->GetViewMatrix()) * glm::vec4(corners[j], 1.0f);
				corners[j] = vW;

				minX = Math::Min(minX, corners[j].x);
				maxX = Math::Max(maxX, corners[j].x);
				
				minY = Math::Min(minY, corners[j].y);
				maxY = Math::Max(maxY, corners[j].y);

				minZ = Math::Min(minZ, corners[j].z);
				maxZ = Math::Max(maxZ, corners[j].z);
			}

			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t j = 0; j < 8; j++)
			{
				frustumCenter += corners[j];
			}
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t j = 0; j < 8; j++) {
				float distance = glm::length(corners[j] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - 3.0f * (lightDir * (-minExtents.z)), frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, 6.0f * maxExtents.z);
			lightOrthoMatrix[1][1] *= -1.0f;

			// Store split distance and matrix in cascade
			cascades[i].splitDepth = (camera->near + splitDist * clipRange) * -1.0f;
			cascades[i].view = lightViewMatrix;
			cascades[i].proj = lightOrthoMatrix;

			lastSplitDist = cascadeSplits[i];
		}


		ubo0.view = cascades[0].view;
		ubo0.proj = cascades[0].proj;
		ubo0.splitPoints = glm::vec4(cascades[0].splitDepth, cascades[1].splitDepth, cascades[2].splitDepth, 0);
		ubo0.config0 = glm::vec4(visualize, contactShadowVisualize, irradianceVisualize, 0.0f);

		ubo1.view = cascades[1].view;
		ubo1.proj = cascades[1].proj;
		ubo1.splitPoints = glm::vec4(cascades[0].splitDepth, cascades[1].splitDepth, cascades[2].splitDepth, 1);

		ubo2.view = cascades[2].view;
		ubo2.proj = cascades[2].proj;
		ubo2.splitPoints = glm::vec4(cascades[0].splitDepth, cascades[1].splitDepth, cascades[2].splitDepth, 2);
	}

	void Render(Scene* scene, glm::vec3 center, Camera* camera, glm::vec4 lightDir)
	{
		GFX::BeginRenderPass(renderPass, 0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

		GFX::SetViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
		GFX::SetScissor(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

		GFX::ApplyPipeline(pipeline0);

		ComputeShadowMatrix(camera, glm::vec3(lightDir));
		GFX::UpdateUniformBuffer(uniform0, 0, &ubo0);
		GFX::BindUniform(uniform0, 0);

		for (auto mesh : scene->meshes)
		{
			GFX::BindVertexBuffer(mesh->vertexBuffer, 0);
			GFX::BindIndexBuffer(mesh->indexBuffer, 0, GFX::IndexType::UInt32);
			GFX::DrawIndexed(mesh->indices.size(), 1, 0);
		}

		GFX::NextSubpass();
		GFX::ApplyPipeline(pipeline1);
		
		GFX::UpdateUniformBuffer(uniform1, 0, &ubo1);
		GFX::BindUniform(uniform1, 0);

		for (auto mesh : scene->meshes)
		{
			GFX::BindVertexBuffer(mesh->vertexBuffer, 0);
			GFX::BindIndexBuffer(mesh->indexBuffer, 0, GFX::IndexType::UInt32);
			GFX::DrawIndexed(mesh->indices.size(), 1, 0);
		}

		GFX::NextSubpass();
		GFX::ApplyPipeline(pipeline2);

		GFX::UpdateUniformBuffer(uniform2, 0, &ubo2);
		GFX::BindUniform(uniform2, 0);

		for (auto mesh : scene->meshes)
		{
			GFX::BindVertexBuffer(mesh->vertexBuffer, 0);
			GFX::BindIndexBuffer(mesh->indexBuffer, 0, GFX::IndexType::UInt32);
			GFX::DrawIndexed(mesh->indices.size(), 1, 0);
		}

		GFX::EndRenderPass();
	}

	ShadowMapUniformObject ubo0 = {};
	ShadowMapUniformObject ubo1 = {};
	ShadowMapUniformObject ubo2 = {};

	GFX::Buffer uniformBuffer0 = {};
	GFX::Buffer uniformBuffer1 = {};
	GFX::Buffer uniformBuffer2 = {};

	GFX::UniformLayout uniformLayout = {};
	GFX::Uniform uniform0 = {};
	GFX::Uniform uniform1 = {};
	GFX::Uniform uniform2 = {};
	
	GFX::Pipeline pipeline0 = {};
	GFX::Pipeline pipeline1 = {};
	GFX::Pipeline pipeline2 = {};

	GFX::Shader vertShader = {};
	GFX::Shader fragShader = {};

	GFX::RenderPass renderPass = {};

	uint32_t m_width = 0;
	uint32_t m_height = 0;

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

	void CreatePipeline()
	{
		GFX::BufferDescription uniformBufferDesc = {};
		uniformBufferDesc.size = sizeof(ShadowMapUniformObject);
		uniformBufferDesc.storageMode = GFX::BufferStorageMode::Dynamic;
		uniformBufferDesc.usage = GFX::BufferUsage::UniformBuffer;

		uniformBuffer0 = GFX::CreateBuffer(uniformBufferDesc);
		uniformBuffer1 = GFX::CreateBuffer(uniformBufferDesc);
		uniformBuffer2 = GFX::CreateBuffer(uniformBufferDesc);

		GFX::VertexBindings vertexBindings = {};
		vertexBindings.SetBindingPosition(0);
		vertexBindings.SetBindingType(GFX::BindingType::Vertex);
		vertexBindings.SetStrideSize(sizeof(Vertex));
		vertexBindings.AddAttribute(0, offsetof(Vertex, position), GFX::ValueType::Float32x3);
		vertexBindings.AddAttribute(1, offsetof(Vertex, normal), GFX::ValueType::Float32x3);
		vertexBindings.AddAttribute(2, offsetof(Vertex, uv), GFX::ValueType::Float32x2);

		GFX::UniformLayoutDescription uniformLayoutDesc = {};
		uniformLayoutDesc.AddUniformBinding(0, GFX::UniformType::UniformBuffer, GFX::ShaderStage::VertexFragment, 1);
		uniformLayout = GFX::CreateUniformLayout(uniformLayoutDesc);

		GFX::UniformDescription uniformDesc0 = {};
		uniformDesc0.SetUniformLayout(uniformLayout);
		uniformDesc0.SetStorageMode(GFX::UniformStorageMode::Dynamic);
		uniformDesc0.AddBufferAttribute(0, uniformBuffer0, 0, sizeof(ShadowMapUniformObject));
		uniform0 = GFX::CreateUniform(uniformDesc0);

		GFX::UniformDescription uniformDesc1 = {};
		uniformDesc1.SetUniformLayout(uniformLayout);
		uniformDesc1.SetStorageMode(GFX::UniformStorageMode::Dynamic);
		uniformDesc1.AddBufferAttribute(0, uniformBuffer1, 0, sizeof(ShadowMapUniformObject));
		uniform1 = GFX::CreateUniform(uniformDesc1);

		GFX::UniformDescription uniformDesc2 = {};
		uniformDesc2.SetUniformLayout(uniformLayout);
		uniformDesc2.SetStorageMode(GFX::UniformStorageMode::Dynamic);
		uniformDesc2.AddBufferAttribute(0, uniformBuffer2, 0, sizeof(ShadowMapUniformObject));
		uniform2 = GFX::CreateUniform(uniformDesc2);

		GFX::UniformBindings uniformBindings = {};
		uniformBindings.AddUniformLayout(uniformLayout);

		GFX::GraphicsPipelineDescription pipelineDesc = {};
		pipelineDesc.enableDepthTest = true;
		pipelineDesc.enableStencilTest = false;
		pipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
		pipelineDesc.renderPass = renderPass;
		pipelineDesc.vertexBindings = vertexBindings;
		pipelineDesc.uniformBindings = uniformBindings;
		pipelineDesc.shaders.push_back(vertShader);
		pipelineDesc.shaders.push_back(fragShader);
		pipelineDesc.cullFace = GFX::CullFace::Front;

		for (int i = 0; i < 1; i++)
		{
			pipelineDesc.blendStates.push_back({});
		}

		pipelineDesc.subpass = 0;
		pipeline0 = GFX::CreatePipeline(pipelineDesc);
		pipelineDesc.subpass = 1;
		pipeline1 = GFX::CreatePipeline(pipelineDesc);
		pipelineDesc.subpass = 2;
		pipeline2 = GFX::CreatePipeline(pipelineDesc);
	}

	void CreateRenderPass()
	{
		GFX::RenderPassDescription renderPassDescription = {};
		renderPassDescription.width = SHADOW_MAP_SIZE;
		renderPassDescription.height = SHADOW_MAP_SIZE;
		
		// Index 0 Shadowmap Depth SHADOWMAP_DEPTH_ATTACHMENT_INDEX
		GFX::AttachmentDescription shadowMapDepthAttachment0 = {};
		shadowMapDepthAttachment0.width = SHADOW_MAP_SIZE;
		shadowMapDepthAttachment0.height = SHADOW_MAP_SIZE;
		shadowMapDepthAttachment0.format = GFX::Format::DEPTH;
		shadowMapDepthAttachment0.type = GFX::AttachmentType::DepthStencil;
		shadowMapDepthAttachment0.loadAction = GFX::AttachmentLoadAction::Clear;
		shadowMapDepthAttachment0.storeAction = GFX::AttachmentStoreAction::Store;
		shadowMapDepthAttachment0.initialLayout = GFX::ImageLayout::Undefined;
		shadowMapDepthAttachment0.finalLayout = GFX::ImageLayout::FragmentShaderRead;

		GFX::ClearValue shadowMapDepthClearColor0 = {};
		shadowMapDepthClearColor0.SetDepth(1.0f);
		shadowMapDepthAttachment0.clearValue = shadowMapDepthClearColor0;

		// Index 1 Shadowmap Depth SHADOWMAP_DEPTH_ATTACHMENT_INDEX
		GFX::AttachmentDescription shadowMapDepthAttachment1 = {};
		shadowMapDepthAttachment1.width = SHADOW_MAP_SIZE;
		shadowMapDepthAttachment1.height = SHADOW_MAP_SIZE;
		shadowMapDepthAttachment1.format = GFX::Format::DEPTH;
		shadowMapDepthAttachment1.type = GFX::AttachmentType::DepthStencil;
		shadowMapDepthAttachment1.loadAction = GFX::AttachmentLoadAction::Clear;
		shadowMapDepthAttachment1.storeAction = GFX::AttachmentStoreAction::Store;
		shadowMapDepthAttachment1.initialLayout = GFX::ImageLayout::Undefined;
		shadowMapDepthAttachment1.finalLayout = GFX::ImageLayout::FragmentShaderRead;

		GFX::ClearValue shadowMapDepthClearColor1 = {};
		shadowMapDepthClearColor1.SetDepth(1.0f);
		shadowMapDepthAttachment1.clearValue = shadowMapDepthClearColor1;

		// Index 2 Shadowmap Depth SHADOWMAP_DEPTH_ATTACHMENT_INDEX
		GFX::AttachmentDescription shadowMapDepthAttachment2 = {};
		shadowMapDepthAttachment2.width = SHADOW_MAP_SIZE;
		shadowMapDepthAttachment2.height = SHADOW_MAP_SIZE;
		shadowMapDepthAttachment2.format = GFX::Format::DEPTH;
		shadowMapDepthAttachment2.type = GFX::AttachmentType::DepthStencil;
		shadowMapDepthAttachment2.loadAction = GFX::AttachmentLoadAction::Clear;
		shadowMapDepthAttachment2.storeAction = GFX::AttachmentStoreAction::Store;
		shadowMapDepthAttachment2.initialLayout = GFX::ImageLayout::Undefined;
		shadowMapDepthAttachment2.finalLayout = GFX::ImageLayout::FragmentShaderRead;

		GFX::ClearValue shadowMapDepthClearColor2 = {};
		shadowMapDepthClearColor2.SetDepth(1.0f);
		shadowMapDepthAttachment2.clearValue = shadowMapDepthClearColor2;

		// Subpass 0, Shadowmap Pass
		GFX::SubPassDescription subpassShadowMap0 = {};
		//subpassShadowMap0.colorAttachments.push_back(0);
		subpassShadowMap0.SetDepthStencilAttachment(0);
		subpassShadowMap0.pipelineType = GFX::PipelineType::Graphics;

		// Subpass 1, Shadowmap Pass
		GFX::SubPassDescription subpassShadowMap1 = {};
		//subpassShadowMap1.colorAttachments.push_back(0);
		subpassShadowMap1.SetDepthStencilAttachment(1);
		subpassShadowMap1.pipelineType = GFX::PipelineType::Graphics;

		// Subpass 2, Shadowmap Pass
		GFX::SubPassDescription subpassShadowMap2 = {};
		//subpassShadowMap2.colorAttachments.push_back(0);
		subpassShadowMap2.SetDepthStencilAttachment(2);
		subpassShadowMap2.pipelineType = GFX::PipelineType::Graphics;

		GFX::DependencyDescription dependencyDesc0 = {};
		dependencyDesc0.srcSubpass = GFX::ExternalSubpass;
		dependencyDesc0.dstSubpass = 0;
		dependencyDesc0.srcStage = GFX::PipelineStage::EarlyFragmentTests;
		dependencyDesc0.dstStage = GFX::PipelineStage::FragmentShader;
		dependencyDesc0.srcAccess = GFX::Access::ColorAttachmentWrite;
		dependencyDesc0.dstAccess = GFX::Access::ShaderRead;

		GFX::DependencyDescription dependencyDesc1 = {};
		dependencyDesc1.srcSubpass = 0;
		dependencyDesc1.dstSubpass = 1;
		dependencyDesc1.srcStage = GFX::PipelineStage::LateFragmentTests;
		dependencyDesc1.dstStage = GFX::PipelineStage::FragmentShader;
		dependencyDesc1.srcAccess = GFX::Access::DepthStencilAttachmentWrite;
		dependencyDesc1.dstAccess = GFX::Access::ShaderRead;

		GFX::DependencyDescription dependencyDesc2 = {};
		dependencyDesc1.srcSubpass = 1;
		dependencyDesc1.dstSubpass = 2;
		dependencyDesc1.srcStage = GFX::PipelineStage::LateFragmentTests;
		dependencyDesc1.dstStage = GFX::PipelineStage::FragmentShader;
		dependencyDesc1.srcAccess = GFX::Access::DepthStencilAttachmentWrite;
		dependencyDesc1.dstAccess = GFX::Access::ShaderRead;

		GFX::DependencyDescription dependencyDesc3 = {};
		dependencyDesc1.srcSubpass = 2;
		dependencyDesc1.dstSubpass = GFX::ExternalSubpass;
		dependencyDesc1.srcStage = GFX::PipelineStage::LateFragmentTests;
		dependencyDesc1.dstStage = GFX::PipelineStage::FragmentShader;
		dependencyDesc1.srcAccess = GFX::Access::DepthStencilAttachmentWrite;
		dependencyDesc1.dstAccess = GFX::Access::ShaderRead;

		renderPassDescription.attachments.push_back(shadowMapDepthAttachment0);
		renderPassDescription.attachments.push_back(shadowMapDepthAttachment1);
		renderPassDescription.attachments.push_back(shadowMapDepthAttachment2);

		//renderPassDescription.dependencies.push_back(dependencyDesc0);
		//renderPassDescription.dependencies.push_back(dependencyDesc1);
		//renderPassDescription.dependencies.push_back(dependencyDesc2);
		//renderPassDescription.dependencies.push_back(dependencyDesc3);

		renderPassDescription.subpasses.push_back(subpassShadowMap0);
		renderPassDescription.subpasses.push_back(subpassShadowMap1);
		renderPassDescription.subpasses.push_back(subpassShadowMap2);

		renderPass = GFX::CreateRenderPass(renderPassDescription);
	}

	ShadowMap()
	{
	}
};