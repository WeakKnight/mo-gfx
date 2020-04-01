#include "modelviewer.h"
#include "spdlog/spdlog.h"
#include <GLFW/glfw3.h>
#include "gfx.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>

const int WIDTH = 800;
const int HEIGHT = 600;

static int s_width = WIDTH;
static int s_height = HEIGHT;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	s_width = width;
	s_height = height;

	spdlog::info("WINDOW Resize");

	// spdlog::info("Window Resize");
	GFX::Resize(width, height);
	// GFX::ResizeRenderPass(renderPass, width, height);
}

struct UniformBufferObject
{
	glm::mat4 view;
	glm::mat4 proj;
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
		vertexBufferDesc.size = sizeof(float) * vertices.size();

		vertexBuffer = GFX::CreateBuffer(vertexBufferDesc);

		GFX::BufferDescription indexBufferDesc = {};
		indexBufferDesc.usage = GFX::BufferUsage::IndexBuffer;
		indexBufferDesc.storageMode = GFX::BufferStorageMode::Static;
		indexBufferDesc.size = sizeof(uint32_t) * indices.size();

		indexBuffer = GFX::CreateBuffer(indexBufferDesc);

		GFX::UpdateBuffer(vertexBuffer, 0, sizeof(float) * vertices.size(), vertices.data());

		gpuResourceInitialized = true;
	}

	void DestroyGPUResources()
	{
		GFX::DestroyBuffer(vertexBuffer);
		GFX::DestroyBuffer(indexBuffer);
	}

	glm::mat4 transform = glm::mat4(1.0f);
	// vec3 pos, vec3 normal, vec2 uv
	std::vector<float> vertices;
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

Scene* LoadScene(const char* path)
{
	Scene* result = new Scene();

	Assimp::Importer meshImporter;
	const aiScene* aiScene = meshImporter.ReadFile(path, aiProcess_Triangulate | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes | aiProcess_GenNormals);

	for (int i = 0; i < aiScene->mNumMeshes; i++)
	{
		aiMesh* aiMesh = aiScene->mMeshes[i];
		Mesh* mesh = new Mesh();
		mesh->name = aiMesh->mName.C_Str();

		mesh->indices.resize(aiMesh->mNumFaces * 3);
		for (size_t f = 0; f < aiMesh->mNumFaces; ++f)
		{
			for (size_t i = 0; i < 3; ++i)
			{
				mesh->indices[f * 3 + i] = aiMesh->mFaces[f].mIndices[i];
			}
		}

		mesh->vertices.resize(aiMesh->mNumVertices * 8);
		for (size_t i = 0; i < aiMesh->mNumVertices; ++i)
		{
			mesh->vertices[i * 8 + 0] = (aiMesh->mVertices[i].x);
			mesh->vertices[i * 8 + 1] = (aiMesh->mVertices[i].y);
			mesh->vertices[i * 8 + 2] = (aiMesh->mVertices[i].z);

			mesh->vertices[i * 8 + 3] = (aiMesh->mNormals[i].x);
			mesh->vertices[i * 8 + 4] = (aiMesh->mNormals[i].y);
			mesh->vertices[i * 8 + 5] = (aiMesh->mNormals[i].z);

			mesh->vertices[i * 8 + 6] = (aiMesh->mTextureCoords[0][i].x);
			mesh->vertices[i * 8 + 7] = (aiMesh->mTextureCoords[0][i].y);
		}

		mesh->CreateGPUResources();

		result->meshes.push_back(mesh);
	}

	return result;
}

void DestroyScene(Scene* scene)
{
	for (auto mesh : scene->meshes)
	{
		delete mesh;
	}

	delete scene;
}

GFX::Pipeline CreateMeshPipeline()
{
	GFX::VertexBindings vertexBindings = {};
	vertexBindings.SetBindingPosition(0);
	vertexBindings.SetBindingType(GFX::BindingType::Vertex);
	vertexBindings.SetStrideSize(8 * sizeof(float));
	vertexBindings.AddAttribute(0, 0, GFX::ValueType::Float32x3);
	vertexBindings.AddAttribute(1, 3 * sizeof(float), GFX::ValueType::Float32x3);
	vertexBindings.AddAttribute(2, 6 * sizeof(float), GFX::ValueType::Float32x2);

	GFX::GraphicsPipelineDescription pipelineDesc = {};
	pipelineDesc.enableDepthTest = true;
	pipelineDesc.enableStencilTest = false;
	pipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
	pipelineDesc.subpass = 0;
	pipelineDesc.vertexBindings = vertexBindings;
}

int main(int, char** args)
{
	InitEnvironment(args);

	App* app = new ModelViewerExample();
	app->Run();

	delete app;

	return 0;
}

static Scene* s_scene = nullptr;

void ModelViewerExample::Init()
{
	spdlog::info("Hello Model Viewer");

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Model Viewer", nullptr, nullptr);

	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

	GFX::InitialDescription initDesc = {};
	initDesc.debugMode = true;
	initDesc.window = m_window;

	GFX::Init(initDesc);

	s_scene = LoadScene("model-viewer/TEST2.fbx");
}

void ModelViewerExample::MainLoop()
{

}

void ModelViewerExample::CleanUp()
{
	DestroyScene(s_scene);
	GFX::Shutdown();
}