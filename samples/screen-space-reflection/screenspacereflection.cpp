#include "screenspacereflection.h"
#include "spdlog/spdlog.h"
#include <GLFW/glfw3.h>
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

#include "string_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "material.h"
#include "mesh.h"

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

const int WIDTH = 800;
const int HEIGHT = 600;

static int s_width = WIDTH;
static int s_height = HEIGHT;

static Scene* s_scene = nullptr;
static ModelUniformBlock* s_modelUniform = nullptr;
GFX::RenderPass s_meshRenderPass;

static PipelineObject* s_meshPipelineObject = nullptr;

static PipelineObject* s_meshMRTPipelineObject = nullptr;
static PipelineObject* s_gatherPipelineObject = nullptr;
static PipelineObject* s_presentPipelineObject = nullptr;

static GFX::UniformLayout s_gatherUniformLayout;
static GFX::Uniform s_gatherUniform;

static GFX::UniformLayout s_presentUniformLayout;
static GFX::Uniform s_presentUniform;

static GFX::Sampler s_nearestSampler;
static GFX::Sampler s_linearSampler;

class Skybox
{
public:
	// Uniforms
	GFX::Uniform uniform;
	GFX::UniformLayout uniformLayout;
	GFX::Buffer uniformBuffer;
	GFX::Image image;
	GFX::Sampler sampler;

	// Model Data
	GFX::Buffer vertexBuffer;

	// Pipeline
	GFX::Shader vertexShader;
	GFX::Shader fragShader;
	GFX::Pipeline pipeline;

	static Skybox* Create()
	{
		Skybox* result = new Skybox();

		// Load And Create Cube Map
		int texWidth, texHeight, texChannels;

		stbi_uc* pixels[6];
		pixels[0] = stbi_load("screen-space-reflection/skybox/Front+Z.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		pixels[1] = stbi_load("screen-space-reflection/skybox/Back-Z.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		pixels[2] = stbi_load("screen-space-reflection/skybox/Up+Y.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		pixels[3] = stbi_load("screen-space-reflection/skybox/Down-Y.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		pixels[5] = stbi_load("screen-space-reflection/skybox/Left+X.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		pixels[4] = stbi_load("screen-space-reflection/skybox/Right-X.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		GFX::ImageDescription imageDescription = {};
		imageDescription.format = GFX::Format::R8G8B8A8;
		imageDescription.width = texWidth;
		imageDescription.height = texHeight;
		imageDescription.depth = 1;
		imageDescription.readOrWriteByCPU = false;
		imageDescription.usage = GFX::ImageUsage::SampledImage;
		imageDescription.type = GFX::ImageType::Cube;
		imageDescription.sampleCount = GFX::ImageSampleCount::Sample1;

		result->image = GFX::CreateImage(imageDescription);

		GFX::BufferDescription stagingBufferDesc = {};
		stagingBufferDesc.size = sizeof(stbi_uc) * texWidth * texHeight * 4 * 6;
		stagingBufferDesc.storageMode = GFX::BufferStorageMode::Dynamic;
		stagingBufferDesc.usage = GFX::BufferUsage::TransferBuffer;

		GFX::Buffer stagingBuffer = GFX::CreateBuffer(stagingBufferDesc);

		for (int i = 0; i < 6; i++)
		{
			GFX::UpdateBuffer(stagingBuffer, i * (sizeof(stbi_uc) * texWidth * texHeight * 4), sizeof(stbi_uc) * texWidth * texHeight * 4, pixels[i]);
		}

		GFX::CopyBufferToImage(result->image, stagingBuffer);
		GFX::DestroyBuffer(stagingBuffer);

		STBI_FREE(pixels[0]);
		STBI_FREE(pixels[1]);
		STBI_FREE(pixels[2]);
		STBI_FREE(pixels[3]);
		STBI_FREE(pixels[4]);
		STBI_FREE(pixels[5]);

		GFX::SamplerDescription samplerDesc = {};
		samplerDesc.maxLod = 10.0f;
		samplerDesc.minFilter = GFX::FilterMode::Linear;
		samplerDesc.magFilter = GFX::FilterMode::Linear;
		samplerDesc.mipmapFilter = GFX::FilterMode::Linear;
		samplerDesc.wrapU = GFX::WrapMode::ClampToEdge;
		samplerDesc.wrapV = GFX::WrapMode::ClampToEdge;

		result->sampler = GFX::CreateSampler(samplerDesc);

		// Mesh Buffer
		GFX::BufferDescription vertexBufferDesc = {};
		vertexBufferDesc.size = sizeof(boxVertices);
		vertexBufferDesc.storageMode = GFX::BufferStorageMode::Static;
		vertexBufferDesc.usage = GFX::BufferUsage::VertexBuffer;

		result->vertexBuffer = GFX::CreateBuffer(vertexBufferDesc);
		GFX::UpdateBuffer(result->vertexBuffer, 0, sizeof(boxVertices), &boxVertices);

		// Uniform Layout
		GFX::UniformLayoutDescription uniformLayoutDesc = {};
		uniformLayoutDesc.AddUniformBinding(0, GFX::UniformType::UniformBuffer, GFX::ShaderStage::Vertex, 1);
		uniformLayoutDesc.AddUniformBinding(1, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);

		result->uniformLayout = GFX::CreateUniformLayout(uniformLayoutDesc);

		// Uniform
		GFX::UniformDescription uniformDesc = {};
		uniformDesc.AddBufferAttribute(0, s_modelUniform->buffer, 0, sizeof(UniformBufferObject));
		uniformDesc.AddImageAttribute(1, result->image, result->sampler);
		uniformDesc.SetUniformLayout(result->uniformLayout);
		uniformDesc.SetStorageMode(GFX::UniformStorageMode::Static);
		result->uniform = GFX::CreateUniform(uniformDesc);

		// Shader
		GFX::ShaderDescription vertShaderDesc = {};
		vertShaderDesc.codes = StringUtils::ReadFile("screen-space-reflection/skybox.vert");
		vertShaderDesc.name = "skybox_vertex";
		vertShaderDesc.stage = GFX::ShaderStage::Vertex;

		result->vertexShader = GFX::CreateShader(vertShaderDesc);

		GFX::ShaderDescription fragShaderDesc = {};
		fragShaderDesc.codes = StringUtils::ReadFile("screen-space-reflection/skybox.frag");
		fragShaderDesc.name = "skybox_frag";
		fragShaderDesc.stage = GFX::ShaderStage::Fragment;

		result->fragShader = GFX::CreateShader(fragShaderDesc);

		// Pipeline
		GFX::VertexBindings vertexBindings = {};
		vertexBindings.AddAttribute(0, 0, GFX::ValueType::Float32x3);
		vertexBindings.SetBindingPosition(0);
		vertexBindings.SetBindingType(GFX::BindingType::Vertex);
		vertexBindings.SetStrideSize(sizeof(float) * 3);

		GFX::UniformBindings uniformBindings = {};
		uniformBindings.AddUniformLayout(result->uniformLayout);

		GFX::GraphicsPipelineDescription pipelineDesc = {};
		pipelineDesc.enableDepthTest = false;
		pipelineDesc.enableStencilTest = false;
		pipelineDesc.cullFace = GFX::CullFace::None;
		pipelineDesc.fronFace = GFX::FrontFace::CounterClockwise;
		pipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
		pipelineDesc.renderPass = s_meshRenderPass;
		pipelineDesc.shaders.push_back(result->vertexShader);
		pipelineDesc.shaders.push_back(result->fragShader);
		pipelineDesc.subpass = 1;
		pipelineDesc.uniformBindings = uniformBindings;
		pipelineDesc.vertexBindings = vertexBindings;

		result->pipeline = GFX::CreatePipeline(pipelineDesc);

		return result;
	}

	~Skybox()
	{
		GFX::DestroyPipeline(pipeline);
		GFX::DestroyShader(vertexShader);
		GFX::DestroyShader(fragShader);
		GFX::DestroyBuffer(vertexBuffer);
		GFX::DestroyBuffer(uniformBuffer);
		GFX::DestroySampler(sampler);
		GFX::DestroyImage(image);
		GFX::DestroyUniformLayout(uniformLayout);
		GFX::DestroyUniform(uniform);
	}
};

static Skybox* skybox = nullptr;

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

GFX::RenderPass CreateScreenSpaceReflectionRenderPass()
{
	// Index 0
	GFX::AttachmentDescription swapChainAttachment = {};
	swapChainAttachment.width = s_width;
	swapChainAttachment.height = s_height;
	swapChainAttachment.format = GFX::Format::SWAPCHAIN;
	swapChainAttachment.loadAction = GFX::AttachmentLoadAction::Clear;
	swapChainAttachment.storeAction = GFX::AttachmentStoreAction::Store;
	swapChainAttachment.type = GFX::AttachmentType::Present;

	GFX::ClearValue swapCahinClearColor = {};
	swapCahinClearColor.SetColor(GFX::Color(0.0f, 0.0f, 0.0f, 1.0f));
	swapChainAttachment.clearValue = swapCahinClearColor;

	// Index 1 Albedo 
	GFX::AttachmentDescription gBufferAlbedoAttachment = {};
	gBufferAlbedoAttachment.width = s_width;
	gBufferAlbedoAttachment.height = s_height;
	gBufferAlbedoAttachment.format = GFX::Format::R8G8B8A8;
	gBufferAlbedoAttachment.loadAction = GFX::AttachmentLoadAction::Clear;
	gBufferAlbedoAttachment.storeAction = GFX::AttachmentStoreAction::DontCare;
	gBufferAlbedoAttachment.type = GFX::AttachmentType::Color;

	GFX::ClearValue albedoClearColor = {};
	albedoClearColor.SetColor(GFX::Color(0.0f, 0.0f, 0.0f, 1.0f));
	gBufferAlbedoAttachment.clearValue = albedoClearColor;

	// index 2 Normal Roughness
	GFX::AttachmentDescription gBufferNormalRoughnessAttachment = {};
	gBufferNormalRoughnessAttachment.width = s_width;
	gBufferNormalRoughnessAttachment.height = s_height;
	gBufferNormalRoughnessAttachment.format = GFX::Format::R8G8B8A8;
	gBufferNormalRoughnessAttachment.loadAction = GFX::AttachmentLoadAction::Clear;
	gBufferNormalRoughnessAttachment.storeAction = GFX::AttachmentStoreAction::DontCare;
	gBufferNormalRoughnessAttachment.type = GFX::AttachmentType::Color;

	GFX::ClearValue normalRoughnessClearColor = {};
	normalRoughnessClearColor.SetColor(GFX::Color(0.0f, 0.0f, 0.0f, 1.0f));
	gBufferNormalRoughnessAttachment.clearValue = normalRoughnessClearColor;

	// Index 3 Position
	GFX::AttachmentDescription gBufferPositionAttachment = {};
	gBufferPositionAttachment.width = s_width;
	gBufferPositionAttachment.height = s_height;
	gBufferPositionAttachment.format = GFX::Format::R16G16B16A16F;
	gBufferPositionAttachment.loadAction = GFX::AttachmentLoadAction::Clear;
	gBufferPositionAttachment.storeAction = GFX::AttachmentStoreAction::DontCare;
	gBufferPositionAttachment.type = GFX::AttachmentType::Color;

	GFX::ClearValue positionClearColor = {};
	positionClearColor.SetColor(GFX::Color(0.0f, 0.0f, 0.0f, 0.0f));
	gBufferPositionAttachment.clearValue = positionClearColor;

	// Index 4 Hdr Attachment
	GFX::AttachmentDescription hdrAttachment = {};
	hdrAttachment.width = s_width;
	hdrAttachment.height = s_height;
	hdrAttachment.format = GFX::Format::R16G16B16A16F;
	hdrAttachment.loadAction = GFX::AttachmentLoadAction::Clear;
	hdrAttachment.storeAction = GFX::AttachmentStoreAction::DontCare;
	hdrAttachment.type = GFX::AttachmentType::Color;

	GFX::ClearValue hdrClearColor = {};
	hdrClearColor.SetColor(GFX::Color(0.0f, 0.0f, 0.0f, 1.0f));
	hdrAttachment.clearValue = hdrClearColor;

	// Index 5 Depth
	GFX::AttachmentDescription depthAttachment = {};
	depthAttachment.width = s_width;
	depthAttachment.height = s_height;
	depthAttachment.format = GFX::Format::DEPTH;
	depthAttachment.type = GFX::AttachmentType::DepthStencil;
	depthAttachment.loadAction = GFX::AttachmentLoadAction::Clear;

	GFX::ClearValue depthClearColor = {};
	depthClearColor.SetDepth(1.0f);
	depthAttachment.clearValue = depthClearColor;

	// Subpass 0, GBufferPass
	GFX::SubPassDescription subpassGBuffer = {};
	// Albedo
	subpassGBuffer.colorAttachments.push_back(1);
	// Normal Roughness
	subpassGBuffer.colorAttachments.push_back(2);
	// Position
	subpassGBuffer.colorAttachments.push_back(3);
	subpassGBuffer.SetDepthStencilAttachment(5);
	subpassGBuffer.pipelineType = GFX::PipelineType::Graphics;
	
	// Subpass 1 Gathering Pass
	GFX::SubPassDescription subpassGather = {};
	// Render To HDR Pass
	subpassGather.colorAttachments.push_back(4);
	//// G Buffer
	//subpassGather.inputAttachments.push_back(1);
	//subpassGather.inputAttachments.push_back(2);
	//subpassGather.inputAttachments.push_back(3);
	subpassGather.pipelineType = GFX::PipelineType::Graphics;

	// Subpass 2 Present
	GFX::SubPassDescription subpassPresent = {};
	// Render To Swapchain
	subpassPresent.colorAttachments.push_back(0);
	//// HDR Input
	//subpassPresent.inputAttachments.push_back(4);
	subpassPresent.pipelineType = GFX::PipelineType::Graphics;

	GFX::DependencyDescription dependencyDesc0 = {};
	dependencyDesc0.srcSubpass = 0;
	dependencyDesc0.dstSubpass = 1;
	dependencyDesc0.srcStage = GFX::PipelineStage::ColorAttachmentOutput;
	dependencyDesc0.dstStage = GFX::PipelineStage::FragmentShader;
	dependencyDesc0.srcAccess = GFX::Access::ColorAttachmentWrite;
	dependencyDesc0.dstAccess = GFX::Access::ShaderRead;

	GFX::DependencyDescription dependencyDesc1 = {};
	dependencyDesc1.srcSubpass = 1;
	dependencyDesc1.dstSubpass = 2;
	dependencyDesc1.srcStage = GFX::PipelineStage::ColorAttachmentOutput;
	dependencyDesc1.dstStage = GFX::PipelineStage::FragmentShader;
	dependencyDesc1.srcAccess = GFX::Access::ColorAttachmentWrite;
	dependencyDesc1.dstAccess = GFX::Access::ShaderRead;

	GFX::RenderPassDescription renderPassDesc = {};
	renderPassDesc.width = s_width;
	renderPassDesc.height = s_height;

	renderPassDesc.attachments.push_back(swapChainAttachment);
	renderPassDesc.attachments.push_back(gBufferAlbedoAttachment);
	renderPassDesc.attachments.push_back(gBufferNormalRoughnessAttachment);
	renderPassDesc.attachments.push_back(gBufferPositionAttachment);
	renderPassDesc.attachments.push_back(hdrAttachment);
	renderPassDesc.attachments.push_back(depthAttachment);

	renderPassDesc.subpasses.push_back(subpassGBuffer);
	renderPassDesc.subpasses.push_back(subpassGather);
	renderPassDesc.subpasses.push_back(subpassPresent);

	renderPassDesc.dependencies.push_back(dependencyDesc0);
	renderPassDesc.dependencies.push_back(dependencyDesc1);

	return GFX::CreateRenderPass(renderPassDesc);
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

void CreateMeshPipeline()
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

	s_meshPipelineObject = new PipelineObject();
	s_meshPipelineObject->Build(s_meshRenderPass, 0, 1, vertexBindings, uniformBindings, "screen-space-reflection/default.vert", "screen-space-reflection/default.frag", true);
}

void CreateMeshMRTPipeline()
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

	s_meshMRTPipelineObject = new PipelineObject();
	s_meshMRTPipelineObject->Build(s_meshRenderPass, 0, 3, vertexBindings, uniformBindings, "screen-space-reflection/default.vert", "screen-space-reflection/defaultMRT.frag", true);
}

void CreateGatheringPipeline()
{
	// Uniform Layout
	GFX::UniformLayoutDescription uniformLayoutDescription = {};
	uniformLayoutDescription.AddUniformBinding(0, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(1, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(2, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);

    s_gatherUniformLayout =	GFX::CreateUniformLayout(uniformLayoutDescription);

	// Uniform
	GFX::UniformDescription uniformDesc = {};
	uniformDesc.AddSampledAttachmentAttribute(0, s_meshRenderPass, 1, s_nearestSampler);
	uniformDesc.AddSampledAttachmentAttribute(1, s_meshRenderPass, 2, s_nearestSampler);
	uniformDesc.AddSampledAttachmentAttribute(2, s_meshRenderPass, 3, s_nearestSampler);

	uniformDesc.SetUniformLayout(s_gatherUniformLayout);
	uniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);

	s_gatherUniform = GFX::CreateUniform(uniformDesc);

	GFX::VertexBindings vertexBindings = {};

	GFX::UniformBindings uniformBindings = {};
	uniformBindings.AddUniformLayout(s_gatherUniformLayout);

	s_gatherPipelineObject = new PipelineObject();
	s_gatherPipelineObject->Build(s_meshRenderPass, 1, 1, vertexBindings, uniformBindings, "screen-space-reflection/screen_quad.vert", "screen-space-reflection/gather_pass.frag", false);
}

void CreatePresentPipeline()
{
	// Uniform Layout
	GFX::UniformLayoutDescription uniformLayoutDescription = {};
	uniformLayoutDescription.AddUniformBinding(0, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);

	s_presentUniformLayout = GFX::CreateUniformLayout(uniformLayoutDescription);

	// Uniform
	GFX::UniformDescription uniformDesc = {};
	// HDR attachment 
	uniformDesc.AddSampledAttachmentAttribute(0, s_meshRenderPass, 4, s_nearestSampler);
	uniformDesc.SetUniformLayout(s_presentUniformLayout);
	uniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);

	s_presentUniform = GFX::CreateUniform(uniformDesc);

	GFX::VertexBindings vertexBindings = {};

	GFX::UniformBindings uniformBindings = {};
	uniformBindings.AddUniformLayout(s_presentUniformLayout);

	s_presentPipelineObject = new PipelineObject();
	s_presentPipelineObject->Build(s_meshRenderPass, 2, 1, vertexBindings, uniformBindings, "screen-space-reflection/screen_quad.vert", "screen-space-reflection/present_pass.frag", false);
}

int main(int, char** args)
{
	InitEnvironment(args);

	App* app = new ScreenSpaceReflectionExample();
	app->Run();

	delete app;

	return 0;
}

void ScreenSpaceReflectionExample::Init()
{
	spdlog::info("Hello Screen Space Reflection");

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Screen Space Reflection", nullptr, nullptr);

	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

	glfwSetCursorPosCallback(m_window, MouseCallback);

	GFX::InitialDescription initDesc = {};
	initDesc.debugMode = true;
	initDesc.window = m_window;

	GFX::Init(initDesc);

	glm::vec3 minP;
	glm::vec3 maxP;
	s_scene = LoadScene("screen-space-reflection/TEST7WithRiver.fbx", minP, maxP);

	target = 0.5f * (minP + maxP);
	radius = 1.5f * Math::Max(Math::Max(maxP.x - minP.x, maxP.y - minP.y), maxP.z - minP.z);
	
	CreateModelUniformBlock("screen-space-reflection/texture.jpg");

	s_meshRenderPass = CreateScreenSpaceReflectionRenderPass();

	GFX::SamplerDescription nearestSamplerDesc = {};
	nearestSamplerDesc.minFilter = GFX::FilterMode::Nearest;
	nearestSamplerDesc.magFilter = GFX::FilterMode::Nearest;
	nearestSamplerDesc.wrapU = GFX::WrapMode::ClampToEdge;
	nearestSamplerDesc.wrapV = GFX::WrapMode::ClampToEdge;
	s_nearestSampler = GFX::CreateSampler(nearestSamplerDesc);

	GFX::SamplerDescription linearSamplerDesc = {};
	linearSamplerDesc.minFilter = GFX::FilterMode::Linear;
	linearSamplerDesc.magFilter = GFX::FilterMode::Linear;
	linearSamplerDesc.wrapU = GFX::WrapMode::ClampToEdge;
	linearSamplerDesc.wrapV = GFX::WrapMode::ClampToEdge;
	s_linearSampler = GFX::CreateSampler(nearestSamplerDesc);
	
	// CreateMeshPipeline();
	CreateMeshMRTPipeline();
	CreateGatheringPipeline();
	CreatePresentPipeline();

	skybox = Skybox::Create();
}

void ScreenSpaceReflectionExample::MainLoop()
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

			GFX::SetViewport(0, 0, s_width, s_height);
			GFX::SetScissor(0, 0, s_width, s_height);

			UniformBufferObject ubo = {};
			ubo.view = GetViewMatrix();
			ubo.proj = glm::perspective(glm::radians(45.0f), (float)s_width / (float)s_height, 0.1f, 1000.0f);
			ubo.proj[1][1] *= -1;

			GFX::UpdateUniformBuffer(s_modelUniform->uniform, 0, &ubo);

			//GFX::ApplyPipeline(skybox->pipeline);
			//// sky box
			//GFX::BindUniform(skybox->uniform, 0);
			//GFX::BindVertexBuffer(skybox->vertexBuffer, 0);
			//GFX::Draw(108, 1, 0, 0);

			GFX::ApplyPipeline(s_meshMRTPipelineObject->pipeline);
			GFX::BindUniform(s_modelUniform->uniform, 0);
			for (auto mesh : s_scene->meshes)
			{
				GFX::BindIndexBuffer(mesh->indexBuffer, 0, GFX::IndexType::UInt32);
				GFX::BindVertexBuffer(mesh->vertexBuffer, 0);
				GFX::DrawIndexed(mesh->indices.size(), 1, 0);
			}

			GFX::NextRenderPass();
			GFX::ApplyPipeline(s_gatherPipelineObject->pipeline);
			GFX::BindUniform(s_gatherUniform, 0);
			GFX::Draw(3, 1, 0, 0);

			GFX::NextRenderPass();
			GFX::ApplyPipeline(s_presentPipelineObject->pipeline);
			GFX::BindUniform(s_presentUniform, 0);
			GFX::Draw(3, 1, 0, 0);

			GFX::EndRenderPass();

			GFX::EndFrame();
		}
	}
}

void ScreenSpaceReflectionExample::CleanUp()
{
	delete skybox;
	DestroyScene(s_scene);
	delete s_modelUniform;

	GFX::DestroyRenderPass(s_meshRenderPass);
	
	PipelineObject::Destroy(s_meshMRTPipelineObject);
	PipelineObject::Destroy(s_gatherPipelineObject);
	PipelineObject::Destroy(s_presentPipelineObject);

	GFX::DestroyUniformLayout(s_gatherUniformLayout);
	GFX::DestroyUniform(s_gatherUniform);

	GFX::DestroyUniformLayout(s_presentUniformLayout);
	GFX::DestroyUniform(s_presentUniform);

	GFX::DestroySampler(s_nearestSampler);
	GFX::DestroySampler(s_linearSampler);

	GFX::Shutdown();
}