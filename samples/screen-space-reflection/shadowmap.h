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

#include "camera.h"

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
		// layer0 near layer 0 far layer 1 near layer 1 far
		glm::vec4 nearFarSettings;
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

	float Layer0Far = 30.0f;

	// return proj matrix for shadow map
	ShadowMapUniformObject ComputeShadowMatrix(Camera* camera, glm::mat4& lightView, float aspect, float near, float far)
	{
		// Calculate 8 near layer corner In Camera Space
		float VFov = glm::radians(camera->fov);
		float HFov = glm::radians(aspect * camera->fov);
		float yn = near * tan(VFov * 0.5);
		float yf = far * tan(VFov * 0.5);
		float xn = near * tan(HFov * 0.5);
		float xf = far * tan(HFov * 0.5);
		float zn = near;
		float zf = far;

		std::vector<glm::vec4> corners;
		corners.reserve(8);
		// near face
		corners.push_back(glm::vec4(xn, yn, zn, 1.0f));
		corners.push_back(glm::vec4(-xn, yn, zn, 1.0f));
		corners.push_back(glm::vec4(xn, -yn, zn, 1.0f));
		corners.push_back(glm::vec4(-xn, -yn, zn, 1.0f));
		// far face
		corners.push_back(glm::vec4(xf, yf, zf, 1.0f));
		corners.push_back(glm::vec4(-xf, yf, zf, 1.0f));
		corners.push_back(glm::vec4(xf, -yf, zf, 1.0f));
		corners.push_back(glm::vec4(-xf, -yf, zf, 1.0f));

		auto camInv = glm::inverse(camera->GetViewMatrix());
		float minX = INFINITY;
		float minY = INFINITY;
		float minZ = INFINITY;
		float maxX = -INFINITY;
		float maxY = -INFINITY;
		float maxZ = -INFINITY;

		// Convert This Coner Points From Camera Space To World Space To Light Space
		for (int i = 0; i < 8; i++)
		{
			auto cornerWorldSpace = camInv * corners[i];
			corners[i] = lightView * cornerWorldSpace;

			minX = Math::Min(minX, corners[i].x);
			minY = Math::Min(minY, corners[i].y);
			minZ = Math::Min(minZ, corners[i].z);

			maxX = Math::Max(maxX, corners[i].x);
			maxY = Math::Max(maxY, corners[i].y);
			maxZ = Math::Max(maxZ, corners[i].z);
		}

		ShadowMapUniformObject result;
		result.proj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
		result.view = lightView;
		result.nearFarSettings = glm::vec4(minZ, maxZ, 0.0f, 0.0f);
		result.proj[1][1] *= -1.0;

		return result;
	}

	void Render(Scene* scene, glm::vec3 center, Camera* camera, float aspect, glm::vec4 lightDir)
	{
		GFX::ApplyPipeline(pipeline);

		// Calculate the new Front vector
		glm::vec3 Front = glm::normalize(lightDir);
		glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
		if (abs(glm::length(Front + WorldUp) - 0.0f) <= 0.00001f)
		{
			WorldUp += glm::vec3(0.0001f, 0.0f, 0.0f);
			WorldUp = glm::normalize(WorldUp);
		}
		// Also re-calculate the Right and Up vector
		glm::vec3 Right = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		glm::vec3 Up = glm::normalize(glm::cross(Right, Front));

		auto lightView = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), Front, Up);

		ShadowMapUniformObject ubo = ComputeShadowMatrix(camera, lightView * glm::eulerAngleXYZ(glm::radians(0.0f), glm::radians(0.0f), glm::radians(0.0f)), aspect, camera->near, Layer0Far);
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

		uniformBuffer = GFX::CreateBuffer(uniformBufferDesc);

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

		GFX::UniformDescription uniformDesc = {};
		uniformDesc.SetUniformLayout(uniformLayout);
		uniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);
		uniformDesc.AddBufferAttribute(0, uniformBuffer, 0, sizeof(ShadowMapUniformObject));
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

		for (int i = 0; i < 1; i++)
		{
			pipelineDesc.blendStates.push_back({});
		}

		pipeline = GFX::CreatePipeline(pipelineDesc);
	}

	ShadowMap()
	{
	}
};