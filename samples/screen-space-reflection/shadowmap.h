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
	}

	struct Cascade
	{
		float splitDepth;
		glm::mat4 proj;
		glm::mat4 view;
	};

	std::array<Cascade, SHADOW_MAP_CASCADE_COUNT> cascades = {};
	float cascadeSplitLambda = 0.9f;

	class ShadowMapUniformObject
	{
	public:
		glm::mat4 view;
		glm::mat4 proj;
		// split 0, split 1, split2, currentIndex
		glm::vec4 splitPoints;
		glm::vec4 nothing;
		glm::vec4 nothing1;
		glm::vec4 nothing2;
	};

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	static ShadowMap* Create(GFX::RenderPass renderPass, uint32_t width, uint32_t height)
	{
		auto result = new ShadowMap();

		result->Resize(width, height);
		result->CreateShader();
		result->CreatePipeline(renderPass);

		return result;
	}

	void Resize(uint32_t width, uint32_t height)
	{
		m_width = width;
		m_height = height;
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
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = cascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] = {
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(camera->GetProjectionMatrix() * camera->GetViewMatrix());
			for (uint32_t i = 0; i < 8; i++) {
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++) {
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++) {
				frustumCenter += frustumCorners[i];
			}
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++) {
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
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

		ubo1.view = cascades[1].view;
		ubo1.proj = cascades[1].proj;
		ubo1.splitPoints = glm::vec4(cascades[0].splitDepth, cascades[1].splitDepth, cascades[2].splitDepth, 1);

		ubo2.view = cascades[2].view;
		ubo2.proj = cascades[2].proj;
		ubo2.splitPoints = glm::vec4(cascades[0].splitDepth, cascades[1].splitDepth, cascades[2].splitDepth, 2);
	}

	void Render(Scene* scene, glm::vec3 center, Camera* camera, float aspect, glm::vec4 lightDir)
	{
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

	void CreatePipeline(GFX::RenderPass renderPass)
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

	ShadowMap()
	{
	}
};