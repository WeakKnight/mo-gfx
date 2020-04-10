#pragma once
#include "gfx.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>

#include "common.h"

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

class Mesh
{
public:
	Mesh()
	{
	}

	~Mesh()
	{
		if (gpuResourceInitialized)
		{
			DestroyGPUResources();
		}
	}

	void CreateGPUResources()
	{
		GFX::BufferDescription vertexBufferDesc = {};
		vertexBufferDesc.usage = GFX::BufferUsage::VertexBuffer;
		vertexBufferDesc.storageMode = GFX::BufferStorageMode::Static;
		vertexBufferDesc.size = sizeof(Vertex) * vertices.size();

		vertexBuffer = GFX::CreateBuffer(vertexBufferDesc);

		GFX::BufferDescription indexBufferDesc = {};
		indexBufferDesc.usage = GFX::BufferUsage::IndexBuffer;
		indexBufferDesc.storageMode = GFX::BufferStorageMode::Static;
		indexBufferDesc.size = sizeof(uint32_t) * indices.size();

		indexBuffer = GFX::CreateBuffer(indexBufferDesc);

		GFX::UpdateBuffer(vertexBuffer, 0, vertexBufferDesc.size, (void*)vertices.data());
		GFX::UpdateBuffer(indexBuffer, 0, indexBufferDesc.size, (void*)indices.data());

		gpuResourceInitialized = true;
	}

	void DestroyGPUResources()
	{
		GFX::DestroyBuffer(vertexBuffer);
		GFX::DestroyBuffer(indexBuffer);
	}

	glm::mat4 transform = glm::mat4(1.0f);
	// vec3 pos, vec3 normal, vec2 uv
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::string name;

	GFX::Buffer vertexBuffer;
	GFX::Buffer indexBuffer;

	bool gpuResourceInitialized = false;
};

class Scene
{
public:
	std::vector<Mesh*> meshes;
};

Scene* LoadScene(const char* path, glm::vec3& min, glm::vec3& max);

void DestroyScene(Scene* scene);