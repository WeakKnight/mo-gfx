#pragma once
#include <gfx.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "string_utils.h"
#include "mesh.h"

class ShadowMap
{
public:
	~ShadowMap()
	{
		GFX::DestroyPipeline(pipeline);
		GFX::DestroyShader(vertShader);
		GFX::DestroyShader(fragShader);
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

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	static ShadowMap* Create(GFX::RenderPass renderPass)
	{
		auto result = new ShadowMap();

		result->CreateShader();
		result->CreatePipeline(renderPass);

		return result;
	}

	void Render(Scene* scene, glm::vec3 lightDir, glm::vec3 center, float cameraNear, float cameraFar)
	{
		GFX::ApplyPipeline(pipeline);
		GFX::SetScissor(0, 0, mapSize, mapSize);
		GFX::SetViewport(0, 0, mapSize, mapSize);

		ShadowMapUniformObject ubo;
		ubo.proj = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, cameraNear, cameraFar);
		ubo.proj[1][1] *= -1;

		GFX::UpdateUniformBuffer(uniform, 0, &ubo);

		GFX::BindUniform(uniform, 0);

		for (auto mesh : scene->meshes)
		{
			GFX::BindVertexBuffer(mesh->vertexBuffer, 0);
			GFX::BindIndexBuffer(mesh->indexBuffer, 0, GFX::IndexType::UInt32);
			GFX::DrawIndexed(mesh->indices.size(), 1, 0);
		}
	}

	GFX::Buffer uniformBuffer = {};
	GFX::UniformLayout uniformLayout = {};
	GFX::Uniform uniform = {};
	GFX::Pipeline pipeline = {};
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

	void CreatePipeline(GFX::RenderPass renderPass)
	{
		GFX::BufferDescription uniformBufferDesc = {};
		uniformBufferDesc.size = GFX::UniformAlign(sizeof(ShadowMapUniformObject));
		uniformBufferDesc.storageMode = GFX::BufferStorageMode::Dynamic;
		uniformBufferDesc.usage = GFX::BufferUsage::UniformBuffer;

		uniformBuffer = GFX::CreateBuffer(uniformBufferDesc);

		GFX::VertexBindings vertexBindings = {};
		vertexBindings.SetBindingPosition(0);
		vertexBindings.SetBindingType(GFX::BindingType::Vertex);
		vertexBindings.SetStrideSize(sizeof(Vertex));
		vertexBindings.AddAttribute(0, offsetof(Vertex, position), GFX::ValueType::Float32x3);
		vertexBindings.AddAttribute(1, offsetof(Vertex, normal), GFX::ValueType::Float32x3);
		vertexBindings.AddAttribute(2, offsetof(Vertex, uv), GFX::ValueType::Float32x2);

		GFX::UniformLayoutDescription uniformLayoutDesc = {};
		uniformLayoutDesc.AddUniformBinding(0, GFX::UniformType::UniformBuffer, GFX::ShaderStage::Vertex, 1);
		uniformLayout = GFX::CreateUniformLayout(uniformLayoutDesc);

		GFX::UniformDescription uniformDesc = {};
		uniformDesc.SetUniformLayout(uniformLayout);
		uniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);
		uniformDesc.AddBufferAttribute(0, uniformBuffer, 0, GFX::UniformAlign(sizeof(ShadowMapUniformObject)));
		uniform = GFX::CreateUniform(uniformDesc);

		GFX::UniformBindings uniformBindings = {};
		uniformBindings.AddUniformLayout(uniformLayout);

		GFX::GraphicsPipelineDescription pipelineDesc = {};
		pipelineDesc.enableDepthTest = true;
		pipelineDesc.enableStencilTest = false;
		pipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
		pipelineDesc.renderPass = renderPass;
		pipelineDesc.subpass = 0;
		pipelineDesc.vertexBindings = vertexBindings;
		pipelineDesc.uniformBindings = uniformBindings;
		pipelineDesc.shaders.push_back(vertShader);
		pipelineDesc.shaders.push_back(fragShader);
		pipelineDesc.cullFace = GFX::CullFace::Front;

		for (int i = 0; i < 0; i++)
		{
			pipelineDesc.blendStates.push_back({});
		}

		pipeline = GFX::CreatePipeline(pipelineDesc);
	}

	ShadowMap()
	{
	}
};