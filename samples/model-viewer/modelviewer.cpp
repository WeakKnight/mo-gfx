#include "modelviewer.h"
#include "spdlog/spdlog.h"
#include <GLFW/glfw3.h>
#include "gfx.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>

#include "string_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class ModelUniformBlock
{
public:
	~ModelUniformBlock()
	{
		GFX::DestroyUniformLayout(uniformLayout);
		GFX::DestroyUniform(uniform);
		GFX::DestroyBuffer(buffer);
		GFX::DestroySampler(sampler);
		GFX::DestroyImage(image);
	}

	GFX::Uniform uniform;
	GFX::UniformLayout uniformLayout;
	GFX::Buffer buffer;
	GFX::Image image;
	GFX::Sampler sampler;
};

struct UniformBufferObject
{
	glm::mat4 view;
	glm::mat4 proj;
};

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

const int WIDTH = 800;
const int HEIGHT = 600;

static int s_width = WIDTH;
static int s_height = HEIGHT;

static Scene* s_scene = nullptr;
static ModelUniformBlock* s_modelUniform = nullptr;
GFX::RenderPass s_meshRenderPass;
GFX::Pipeline s_meshPipeline;
GFX::Shader s_meshVertShader;
GFX::Shader s_meshFragShader;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	s_width = width;
	s_height = height;

	// spdlog::info("Window Resize");
	GFX::Resize(width, height);
	GFX::ResizeRenderPass(s_meshRenderPass, width, height);
}

void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		
	}
}

static bool firstMouseCapture = false;
static float lastX = 0.0f;
static float lastY = 0.0f;
static glm::vec2 mouseOffset;
static glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
static float theta;
static float beta;
static float radius = 10.0f;

constexpr float THETA_SPEED = 1.5f;
constexpr float PI = 3.1415927f;

float clamp(float input, float left, float right)
{
	if (input < left)
	{
		return left;
	}
	if (input > right)
	{
		return right;
	}

	return input;
}

void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouseCapture)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouseCapture = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	mouseOffset = glm::vec2(xoffset, yoffset);

	lastX = xpos;
	lastY = ypos;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		theta = clamp(theta + -1.0f * mouseOffset.x * deltaTime * THETA_SPEED, 0.0f, 2.0f * PI);
		beta = clamp(beta + -1.0f * mouseOffset.y * deltaTime * THETA_SPEED, -0.5f * PI, 0.5f * PI);
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		radius = clamp(radius + mouseOffset.y * deltaTime * 100.0f, 5.0f, 999999.0f);
	}
}

glm::mat4 GetViewMatrix()
{
	float cameraY = sin(beta) * radius;
	float horizontalRadius = cos(beta) * radius;

	float cameraX = sin(theta) * horizontalRadius;
	float cameraZ = cos(theta) * horizontalRadius;

	auto up = glm::normalize(glm::cross(glm::cross(glm::vec3(cameraX, 0.0f, cameraZ), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(cameraX, cameraY, cameraZ)));

	return glm::lookAt(target + glm::vec3(cameraX, cameraY, cameraZ), target, up);
}

Scene* LoadScene(const char* path)
{
	Scene* result = new Scene();

	Assimp::Importer meshImporter;
	const aiScene* aiScene = meshImporter.ReadFile(path, aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes);

	float minX = INFINITY;
	float minY = INFINITY;
	float minZ = INFINITY;
	float maxX = -INFINITY;
	float maxY = -INFINITY;
	float maxZ = -INFINITY;

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

		for (size_t j = 0; j < aiMesh->mNumVertices; ++j)
		{
			Vertex vertex;
			
			vertex.position = glm::vec3(aiMesh->mVertices[j].x, aiMesh->mVertices[j].y, aiMesh->mVertices[j].z);
			vertex.normal = glm::vec3(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z);
			vertex.uv = glm::vec2(aiMesh->mTextureCoords[0][j].x, 1.0f - aiMesh->mTextureCoords[0][j].y);

			minX = Math::Min(vertex.position.x, minX);
			minY = Math::Min(vertex.position.y, minY);
			minZ = Math::Min(vertex.position.z, minZ);

			maxX = Math::Max(vertex.position.x, maxX);
			maxY = Math::Max(vertex.position.y, maxY);
			maxZ = Math::Max(vertex.position.z, maxZ);

			mesh->vertices.push_back(vertex);
		}

		target = 0.5f * (glm::vec3(minX, minY, minZ) + glm::vec3(maxX, maxY, maxZ));
		radius = 1.5f * Math::Max(Math::Max(maxX - minX, maxY - minY), maxZ - minZ);

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

void CreateModelUniformBlock(const char* texPath)
{
	s_modelUniform = new ModelUniformBlock();

	GFX::UniformLayoutDescription uniformLayoutDesc = {};
	// UBO
	uniformLayoutDesc.AddUniformBinding(0, GFX::UniformType::UniformBuffer, GFX::ShaderStage::Vertex, 1);
	// Texture
	uniformLayoutDesc.AddUniformBinding(1, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);

	s_modelUniform->uniformLayout = GFX::CreateUniformLayout(uniformLayoutDesc);

	GFX::BufferDescription bufferDesc = {};
	bufferDesc.size = sizeof(UniformBufferObject);
	bufferDesc.storageMode = GFX::BufferStorageMode::Dynamic;
	bufferDesc.usage = GFX::BufferUsage::UniformBuffer;
	s_modelUniform->buffer = GFX::CreateBuffer(bufferDesc);

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(texPath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	GFX::ImageDescription imageDescription = {};
	imageDescription.format = GFX::Format::R8G8B8A8;
	imageDescription.width = texWidth;
	imageDescription.height = texHeight;
	imageDescription.depth = 1;
	imageDescription.readOrWriteByCPU = false;
	imageDescription.usage = GFX::ImageUsage::SampledImage;
	imageDescription.type = GFX::ImageType::Image2D;
	imageDescription.sampleCount = GFX::ImageSampleCount::Sample1;

	s_modelUniform->image = GFX::CreateImage(imageDescription);
	GFX::UpdateImageMemory(s_modelUniform->image, pixels, sizeof(stbi_uc) * texWidth * texHeight * 4);

	STBI_FREE(pixels);

	// s_modelUniform->image = GFX::CreateImageFromKtxTexture(texPath);

	GFX::SamplerDescription samplerDesc = {};
	samplerDesc.maxLod = 10.0f;
	samplerDesc.minFilter = GFX::FilterMode::Linear;
	samplerDesc.magFilter = GFX::FilterMode::Linear;
	samplerDesc.mipmapFilter = GFX::FilterMode::Linear;
	samplerDesc.wrapU = GFX::WrapMode::Repeat;
	samplerDesc.wrapV = GFX::WrapMode::Repeat;
	
	s_modelUniform->sampler = GFX::CreateSampler(samplerDesc);

	GFX::UniformDescription uniformDesc = {};
	uniformDesc.SetStorageMode(GFX::UniformStorageMode::Static);
	uniformDesc.SetUniformLayout(s_modelUniform->uniformLayout);
	uniformDesc.AddBufferAttribute(0, s_modelUniform->buffer, 0, sizeof(UniformBufferObject));
	uniformDesc.AddImageAttribute(1, s_modelUniform->image, s_modelUniform->sampler);
	
	s_modelUniform->uniform = GFX::CreateUniform(uniformDesc);
}

GFX::RenderPass CreateRenderPass()
{
	// Index 0
	GFX::AttachmentDescription swapChainAttachment = {};
	swapChainAttachment.width = s_width;
	swapChainAttachment.height = s_height;
	swapChainAttachment.format = GFX::Format::SWAPCHAIN;
	swapChainAttachment.loadAction = GFX::AttachmentLoadAction::Clear;
	swapChainAttachment.storeAction = GFX::AttachmentStoreAction::Store;
	swapChainAttachment.type = GFX::AttachmentType::Present;

	GFX::ClearValue colorClearColor = {};
	colorClearColor.SetColor(GFX::Color(0.0f, 0.0f, 0.0f, 1.0f));
	swapChainAttachment.clearValue = colorClearColor;

	// Index 1
	GFX::AttachmentDescription depthAttachment = {};
	depthAttachment.width = s_width;
	depthAttachment.height = s_height;
	depthAttachment.format = GFX::Format::DEPTH;
	depthAttachment.type = GFX::AttachmentType::DepthStencil;
	depthAttachment.loadAction = GFX::AttachmentLoadAction::Clear;

	GFX::ClearValue depthClearColor = {};
	depthClearColor.SetDepth(1.0f);
	depthAttachment.clearValue = depthClearColor;

	GFX::SubPassDescription subPassSwapChain = {};
	subPassSwapChain.colorAttachments.push_back(0);
	subPassSwapChain.SetDepthStencilAttachment(1);
	subPassSwapChain.pipelineType = GFX::PipelineType::Graphics;

	GFX::RenderPassDescription renderPassDesc = {};
	renderPassDesc.width = s_width;
	renderPassDesc.height = s_height;
	
	renderPassDesc.attachments.push_back(swapChainAttachment);
	renderPassDesc.attachments.push_back(depthAttachment);

	renderPassDesc.subpasses.push_back(subPassSwapChain);

	return GFX::CreateRenderPass(renderPassDesc);
}

GFX::Pipeline CreateMeshPipeline()
{
	GFX::VertexBindings vertexBindings = {};
	vertexBindings.SetBindingPosition(0);
	vertexBindings.SetBindingType(GFX::BindingType::Vertex);
	vertexBindings.SetStrideSize(sizeof(Vertex));
	vertexBindings.AddAttribute(0, offsetof(Vertex, position), GFX::ValueType::Float32x3);
	vertexBindings.AddAttribute(1, offsetof(Vertex, normal), GFX::ValueType::Float32x3);
	vertexBindings.AddAttribute(2, offsetof(Vertex, uv), GFX::ValueType::Float32x2);

	GFX::UniformBindings uniformBindings = {};
	uniformBindings.AddUniformLayout(s_modelUniform->uniformLayout);

	GFX::ShaderDescription vertDesc = {};
	vertDesc.name = "default";
	vertDesc.codes = StringUtils::ReadFile("model-viewer/default.vert");
	vertDesc.stage = GFX::ShaderStage::Vertex;

	s_meshVertShader = GFX::CreateShader(vertDesc);

	GFX::ShaderDescription fragDesc = {};
	fragDesc.name = "default";
	fragDesc.codes = StringUtils::ReadFile("model-viewer/default.frag");
	fragDesc.stage = GFX::ShaderStage::Fragment;

	s_meshFragShader = GFX::CreateShader(fragDesc);

	GFX::GraphicsPipelineDescription pipelineDesc = {};
	pipelineDesc.enableDepthTest = true;
	pipelineDesc.enableStencilTest = false;
	pipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
	pipelineDesc.renderPass = s_meshRenderPass;
	pipelineDesc.subpass = 0;
	pipelineDesc.vertexBindings = vertexBindings;
	pipelineDesc.uniformBindings = uniformBindings;
	pipelineDesc.shaders.push_back(s_meshVertShader);
	pipelineDesc.shaders.push_back(s_meshFragShader);

	return GFX::CreatePipeline(pipelineDesc);
}

int main(int, char** args)
{
	InitEnvironment(args);

	App* app = new ModelViewerExample();
	app->Run();

	delete app;

	return 0;
}

void ModelViewerExample::Init()
{
	spdlog::info("Hello Model Viewer");

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Model Viewer", nullptr, nullptr);

	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

	glfwSetKeyCallback(m_window, KeyCallback);
	glfwSetCursorPosCallback(m_window, MouseCallback);

	GFX::InitialDescription initDesc = {};
	initDesc.debugMode = true;
	initDesc.window = m_window;

	GFX::Init(initDesc);

	s_scene = 
		// LoadScene("model-viewer/carved_pillar.fbx");
		LoadScene("model-viewer/TEST2.fbx");
	
	// CreateModelUniformBlock("model-viewer/carved_pillar_Albedo.jpg");
	CreateModelUniformBlock("model-viewer/texture.jpg");
	
	s_meshRenderPass = CreateRenderPass();
	s_meshPipeline = CreateMeshPipeline();
}

void ModelViewerExample::MainLoop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (GFX::BeginFrame())
		{
			GFX::BeginRenderPass(s_meshRenderPass, 0, 0, s_width, s_height);

			GFX::ApplyPipeline(s_meshPipeline);

			GFX::SetViewport(0, 0, s_width, s_height);
			GFX::SetScissor(0, 0, s_width, s_height);

			UniformBufferObject ubo = {};
			ubo.view = GetViewMatrix();
				// glm::lookAt(glm::vec3(2.0f, 26.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			ubo.proj = glm::perspective(glm::radians(45.0f), (float)s_width / (float)s_height, 0.1f, 1000.0f);
			ubo.proj[1][1] *= -1;

			GFX::UpdateUniformBuffer(s_modelUniform->uniform, 0, &ubo);

			GFX::BindUniform(s_modelUniform->uniform, 0);
			for (auto mesh : s_scene->meshes)
			{
				GFX::BindIndexBuffer(mesh->indexBuffer, 0, GFX::IndexType::UInt32);
				GFX::BindVertexBuffer(mesh->vertexBuffer, 0);
				GFX::DrawIndexed(mesh->indices.size(), 1, 0);
			}

			GFX::EndRenderPass();

			GFX::EndFrame();
		}
	}
}

void ModelViewerExample::CleanUp()
{
	DestroyScene(s_scene);
	delete s_modelUniform;
	GFX::DestroyPipeline(s_meshPipeline);
	GFX::DestroyRenderPass(s_meshRenderPass);

	GFX::DestroyShader(s_meshVertShader);
	GFX::DestroyShader(s_meshFragShader);

	GFX::Shutdown();
}