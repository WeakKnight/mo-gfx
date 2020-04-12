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

#include "shadowmap.h"
#include "camera.h"

class ModelUniformBlock
{
public:
	~ModelUniformBlock()
	{
		GFX::DestroyUniformLayout(uniformLayout);
		GFX::DestroyUniform(uniform);
		GFX::DestroyBuffer(buffer);
		GFX::DestroySampler(sampler);
		GFX::DestroyImage(albedoImage);
		GFX::DestroyImage(roughnessImage);
	}

	GFX::Uniform uniform;
	GFX::UniformLayout uniformLayout;
	GFX::Buffer buffer;
	GFX::Image albedoImage;
	GFX::Image roughnessImage;
	GFX::Sampler sampler;
};

struct UniformBufferObject
{
	glm::mat4 view;
	glm::mat4 proj;
};

struct GatheringPassUniformData
{
	glm::vec4 lightDir;
	glm::vec4 lightColor;
	glm::mat4 view;
	glm::mat4 proj;
};

const int WIDTH = 800;
const int HEIGHT = 600;

static int s_width = WIDTH;
static int s_height = HEIGHT;

static Scene* s_scene = nullptr;
static ModelUniformBlock* s_modelUniform = nullptr;
static ModelUniformBlock* s_waterUniform = nullptr;

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

static GFX::Buffer s_gatheringPassUniformBuffer;
static GFX::Image s_irradianceMap;

static ShadowMap* s_shadowMap;

static Camera* s_camera = nullptr;

#define ALBEDO_ATTACHMENT_INDEX 1
#define NORMAL_ATTACHMENT_INDEX 2
#define HDR_ATTACHMENT_INDEX 3
#define DEPTH_ATTACHMENT_INDEX 4
#define SHADOWMAP_DEPTH_ATTACHMENT_0_INDEX 5
#define SHADOWMAP_DEPTH_ATTACHMENT_1_INDEX 6
#define SHADOWMAP_DEPTH_ATTACHMENT_2_INDEX 7

#define SHADOWMAP_PASS_0_INDEX 0
#define SHADOWMAP_PASS_1_INDEX 1
#define SHADOWMAP_PASS_2_INDEX 2
#define MRT_PASS_INDEX 3
#define GATHER_PASS_INDEX 4
#define PRESENT_PASS_INDEX 5

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

	static GFX::Image LoadCubeMap(std::vector<std::string>& textureNames)
	{
		GFX::Image result = {};
		// Load And Create Cube Map
		int texWidth, texHeight, texChannels;

		stbi_uc* pixels[6];
		pixels[0] = stbi_load(textureNames[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		pixels[1] = stbi_load(textureNames[1].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		pixels[2] = stbi_load(textureNames[2].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		pixels[3] = stbi_load(textureNames[3].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		pixels[5] = stbi_load(textureNames[4].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		pixels[4] = stbi_load(textureNames[5].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		GFX::ImageDescription imageDescription = {};
		imageDescription.format = GFX::Format::R8G8B8A8;
		imageDescription.width = texWidth;
		imageDescription.height = texHeight;
		imageDescription.depth = 1;
		imageDescription.readOrWriteByCPU = false;
		imageDescription.usage = GFX::ImageUsage::SampledImage;
		imageDescription.type = GFX::ImageType::Cube;
		imageDescription.sampleCount = GFX::ImageSampleCount::Sample1;

		result = GFX::CreateImage(imageDescription);

		GFX::BufferDescription stagingBufferDesc = {};
		stagingBufferDesc.size = sizeof(stbi_uc) * texWidth * texHeight * 4 * 6;
		stagingBufferDesc.storageMode = GFX::BufferStorageMode::Dynamic;
		stagingBufferDesc.usage = GFX::BufferUsage::TransferBuffer;

		GFX::Buffer stagingBuffer = GFX::CreateBuffer(stagingBufferDesc);

		for (int i = 0; i < 6; i++)
		{
			GFX::UpdateBuffer(stagingBuffer, i * (sizeof(stbi_uc) * texWidth * texHeight * 4), sizeof(stbi_uc) * texWidth * texHeight * 4, pixels[i]);
		}

		GFX::CopyBufferToImage(result, stagingBuffer);
		GFX::DestroyBuffer(stagingBuffer);

		STBI_FREE(pixels[0]);
		STBI_FREE(pixels[1]);
		STBI_FREE(pixels[2]);
		STBI_FREE(pixels[3]);
		STBI_FREE(pixels[4]);
		STBI_FREE(pixels[5]);

		return result;
	}

	static Skybox* Create()
	{
		Skybox* result = new Skybox();

		std::vector<std::string> textureNames;
		textureNames.reserve(6);
		textureNames.push_back("screen-space-reflection/skybox/Front+Z.png");
		textureNames.push_back("screen-space-reflection/skybox/Back-Z.png");
		textureNames.push_back("screen-space-reflection/skybox/Up+Y.png");
		textureNames.push_back("screen-space-reflection/skybox/Down-Y.png");
		textureNames.push_back("screen-space-reflection/skybox/Left+X.png");
		textureNames.push_back("screen-space-reflection/skybox/Right-X.png");

		result->image = LoadCubeMap(textureNames);

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
		pipelineDesc.subpass = GATHER_PASS_INDEX;
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
static glm::vec3 target;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	s_width = width;
	s_height = height;

	// spdlog::info("Window Resize");
	GFX::Resize(width, height);
	GFX::ResizeRenderPass(s_meshRenderPass, width, height);
	s_shadowMap->Resize(width, height);

	// Recreate Attachment Relavant Uniform
	
	GFX::DestroyUniform(s_gatherUniform);
	GFX::UniformDescription gatherUniformDesc = {};
	gatherUniformDesc.AddInputAttachmentAttribute(0, s_meshRenderPass, ALBEDO_ATTACHMENT_INDEX);
	gatherUniformDesc.AddInputAttachmentAttribute(1, s_meshRenderPass, NORMAL_ATTACHMENT_INDEX);
	gatherUniformDesc.AddBufferAttribute(2, s_gatheringPassUniformBuffer, 0, sizeof(GatheringPassUniformData));
	gatherUniformDesc.AddImageAttribute(3, skybox->image, skybox->sampler);
	gatherUniformDesc.AddImageAttribute(4, s_irradianceMap, skybox->sampler);
	gatherUniformDesc.AddSampledAttachmentAttribute(5, s_meshRenderPass, DEPTH_ATTACHMENT_INDEX, s_nearestSampler);
	gatherUniformDesc.AddSampledAttachmentAttribute(6, s_meshRenderPass, SHADOWMAP_DEPTH_ATTACHMENT_0_INDEX, s_nearestSampler);
	gatherUniformDesc.AddSampledAttachmentAttribute(7, s_meshRenderPass, SHADOWMAP_DEPTH_ATTACHMENT_1_INDEX, s_nearestSampler);
	gatherUniformDesc.AddSampledAttachmentAttribute(8, s_meshRenderPass, SHADOWMAP_DEPTH_ATTACHMENT_2_INDEX, s_nearestSampler);

	gatherUniformDesc.SetUniformLayout(s_gatherUniformLayout);
	gatherUniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);
	s_gatherUniform = GFX::CreateUniform(gatherUniformDesc);

	GFX::DestroyUniform(s_presentUniform);
	GFX::UniformDescription presentUniformDesc = {};
	presentUniformDesc.AddSampledAttachmentAttribute(0, s_meshRenderPass, HDR_ATTACHMENT_INDEX, s_nearestSampler);
	presentUniformDesc.SetUniformLayout(s_presentUniformLayout);
	presentUniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);
	s_presentUniform = GFX::CreateUniform(presentUniformDesc);
}

float lastX = s_width / 2.0f;
float lastY = s_height / 2.0f;
bool firstMouse = true;

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
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	if (s_camera != nullptr)
	{
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			s_camera->ProcessMouseMovement(xoffset, yoffset);
		}
	}
}

ModelUniformBlock* CreateModelUniformBlock(const char* albedoTexPath, const char* roughnessTexPath)
{
	auto result = new ModelUniformBlock();

	GFX::UniformLayoutDescription uniformLayoutDesc = {};
	// UBO
	uniformLayoutDesc.AddUniformBinding(0, GFX::UniformType::UniformBuffer, GFX::ShaderStage::Vertex, 1);
	// Texture
	uniformLayoutDesc.AddUniformBinding(1, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDesc.AddUniformBinding(2, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);

	result->uniformLayout = GFX::CreateUniformLayout(uniformLayoutDesc);

	GFX::BufferDescription bufferDesc = {};
	bufferDesc.size = sizeof(UniformBufferObject);
	bufferDesc.storageMode = GFX::BufferStorageMode::Dynamic;
	bufferDesc.usage = GFX::BufferUsage::UniformBuffer;
	result->buffer = GFX::CreateBuffer(bufferDesc);

	int texWidth, texHeight, texChannels;
	stbi_uc* albedoPixels = stbi_load(albedoTexPath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	GFX::ImageDescription albedoImageDescription = {};
	albedoImageDescription.format = GFX::Format::R8G8B8A8;
	albedoImageDescription.width = texWidth;
	albedoImageDescription.height = texHeight;
	albedoImageDescription.depth = 1;
	albedoImageDescription.readOrWriteByCPU = false;
	albedoImageDescription.usage = GFX::ImageUsage::SampledImage;
	albedoImageDescription.type = GFX::ImageType::Image2D;
	albedoImageDescription.sampleCount = GFX::ImageSampleCount::Sample1;

	result->albedoImage = GFX::CreateImage(albedoImageDescription);
	GFX::UpdateImageMemory(result->albedoImage, albedoPixels, sizeof(stbi_uc) * texWidth * texHeight * 4);

	STBI_FREE(albedoPixels);

	stbi_uc* roughnessPixels = stbi_load(roughnessTexPath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	GFX::ImageDescription roughnessImageDescription = {};
	roughnessImageDescription.format = GFX::Format::R8G8B8A8;
	roughnessImageDescription.width = texWidth;
	roughnessImageDescription.height = texHeight;
	roughnessImageDescription.depth = 1;
	roughnessImageDescription.readOrWriteByCPU = false;
	roughnessImageDescription.usage = GFX::ImageUsage::SampledImage;
	roughnessImageDescription.type = GFX::ImageType::Image2D;
	roughnessImageDescription.sampleCount = GFX::ImageSampleCount::Sample1;

	result->roughnessImage = GFX::CreateImage(roughnessImageDescription);
	GFX::UpdateImageMemory(result->roughnessImage, roughnessPixels, sizeof(stbi_uc) * texWidth * texHeight * 4);

	STBI_FREE(roughnessPixels);

	// s_modelUniform->image = GFX::CreateImageFromKtxTexture(texPath);

	GFX::SamplerDescription samplerDesc = {};
	samplerDesc.maxLod = 10.0f;
	samplerDesc.minFilter = GFX::FilterMode::Linear;
	samplerDesc.magFilter = GFX::FilterMode::Linear;
	samplerDesc.mipmapFilter = GFX::FilterMode::Linear;
	samplerDesc.wrapU = GFX::WrapMode::ClampToEdge;
	samplerDesc.wrapV = GFX::WrapMode::ClampToEdge;
	
	result->sampler = GFX::CreateSampler(samplerDesc);

	GFX::UniformDescription uniformDesc = {};
	uniformDesc.SetStorageMode(GFX::UniformStorageMode::Static);
	uniformDesc.SetUniformLayout(result->uniformLayout);
	uniformDesc.AddBufferAttribute(0, result->buffer, 0, sizeof(UniformBufferObject));
	uniformDesc.AddImageAttribute(1, result->albedoImage, result->sampler);
	uniformDesc.AddImageAttribute(2, result->roughnessImage, result->sampler);
	
	result->uniform = GFX::CreateUniform(uniformDesc);
	
	return result;
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
	gBufferAlbedoAttachment.storeAction = GFX::AttachmentStoreAction::Store;
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
	gBufferNormalRoughnessAttachment.storeAction = GFX::AttachmentStoreAction::Store;
	gBufferNormalRoughnessAttachment.type = GFX::AttachmentType::Color;

	GFX::ClearValue normalRoughnessClearColor = {};
	normalRoughnessClearColor.SetColor(GFX::Color(0.0f, 0.0f, 0.0f, 1.0f));
	gBufferNormalRoughnessAttachment.clearValue = normalRoughnessClearColor;

	// Index 3 Hdr Attachment
	GFX::AttachmentDescription hdrAttachment = {};
	hdrAttachment.width = s_width;
	hdrAttachment.height = s_height;
	hdrAttachment.format = GFX::Format::R16G16B16A16F;
	hdrAttachment.loadAction = GFX::AttachmentLoadAction::Clear;
	hdrAttachment.storeAction = GFX::AttachmentStoreAction::Store;
	hdrAttachment.type = GFX::AttachmentType::Color;

	GFX::ClearValue hdrClearColor = {};
	hdrClearColor.SetColor(GFX::Color(0.0f, 0.0f, 0.0f, 1.0f));
	hdrAttachment.clearValue = hdrClearColor;

	// Index 4 Depth
	GFX::AttachmentDescription depthAttachment = {};
	depthAttachment.width = s_width;
	depthAttachment.height = s_height;
	depthAttachment.format = GFX::Format::DEPTH;
	depthAttachment.type = GFX::AttachmentType::DepthStencil;
	depthAttachment.loadAction = GFX::AttachmentLoadAction::Clear;
	// depthAttachment.storeAction = GFX::AttachmentStoreAction::Store;

	GFX::ClearValue depthClearColor = {};
	depthClearColor.SetDepth(1.0f);
	depthAttachment.clearValue = depthClearColor;

	// Index 5 Shadowmap Depth SHADOWMAP_DEPTH_ATTACHMENT_INDEX
	GFX::AttachmentDescription shadowMapDepthAttachment0 = {};
	shadowMapDepthAttachment0.width = s_width;
	shadowMapDepthAttachment0.height = s_height;
	shadowMapDepthAttachment0.format = GFX::Format::DEPTH;
	shadowMapDepthAttachment0.type = GFX::AttachmentType::DepthStencil;
	shadowMapDepthAttachment0.loadAction = GFX::AttachmentLoadAction::Clear;

	GFX::ClearValue shadowMapDepthClearColor0 = {};
	shadowMapDepthClearColor0.SetDepth(1.0f);
	shadowMapDepthAttachment0.clearValue = shadowMapDepthClearColor0;

	// Index 6 Shadowmap Depth SHADOWMAP_DEPTH_ATTACHMENT_INDEX
	GFX::AttachmentDescription shadowMapDepthAttachment1 = {};
	shadowMapDepthAttachment1.width = s_width;
	shadowMapDepthAttachment1.height = s_height;
	shadowMapDepthAttachment1.format = GFX::Format::DEPTH;
	shadowMapDepthAttachment1.type = GFX::AttachmentType::DepthStencil;
	shadowMapDepthAttachment1.loadAction = GFX::AttachmentLoadAction::Clear;

	GFX::ClearValue shadowMapDepthClearColor1 = {};
	shadowMapDepthClearColor1.SetDepth(1.0f);
	shadowMapDepthAttachment1.clearValue = shadowMapDepthClearColor1;

	// Index 7 Shadowmap Depth SHADOWMAP_DEPTH_ATTACHMENT_INDEX
	GFX::AttachmentDescription shadowMapDepthAttachment2 = {};
	shadowMapDepthAttachment2.width = s_width;
	shadowMapDepthAttachment2.height = s_height;
	shadowMapDepthAttachment2.format = GFX::Format::DEPTH;
	shadowMapDepthAttachment2.type = GFX::AttachmentType::DepthStencil;
	shadowMapDepthAttachment2.loadAction = GFX::AttachmentLoadAction::Clear;

	GFX::ClearValue shadowMapDepthClearColor2 = {};
	shadowMapDepthClearColor2.SetDepth(1.0f);
	shadowMapDepthAttachment2.clearValue = shadowMapDepthClearColor2;

	// Subpass 0, Shadowmap Pass
	GFX::SubPassDescription subpassShadowMap0 = {};
	//subpassShadowMap0.colorAttachments.push_back(0);
	subpassShadowMap0.SetDepthStencilAttachment(SHADOWMAP_DEPTH_ATTACHMENT_0_INDEX);
	subpassShadowMap0.pipelineType = GFX::PipelineType::Graphics;

	// Subpass 1, Shadowmap Pass
	GFX::SubPassDescription subpassShadowMap1 = {};
	//subpassShadowMap1.colorAttachments.push_back(0);
	subpassShadowMap1.SetDepthStencilAttachment(SHADOWMAP_DEPTH_ATTACHMENT_1_INDEX);
	subpassShadowMap1.pipelineType = GFX::PipelineType::Graphics;

	// Subpass 2, Shadowmap Pass
	GFX::SubPassDescription subpassShadowMap2 = {};
	//subpassShadowMap2.colorAttachments.push_back(0);
	subpassShadowMap2.SetDepthStencilAttachment(SHADOWMAP_DEPTH_ATTACHMENT_2_INDEX);
	subpassShadowMap2.pipelineType = GFX::PipelineType::Graphics;

	// Subpass 3, GBufferPass
	GFX::SubPassDescription subpassGBuffer = {};
	// Albedo
	subpassGBuffer.colorAttachments.push_back(ALBEDO_ATTACHMENT_INDEX);
	// Normal Roughness
	subpassGBuffer.colorAttachments.push_back(NORMAL_ATTACHMENT_INDEX);

	subpassGBuffer.SetDepthStencilAttachment(DEPTH_ATTACHMENT_INDEX);
	subpassGBuffer.pipelineType = GFX::PipelineType::Graphics;
	
	// Subpass 4 Gathering Pass
	GFX::SubPassDescription subpassGather = {};
	// Render To HDR Pass
	subpassGather.colorAttachments.push_back(HDR_ATTACHMENT_INDEX);
	// G Buffer
	subpassGather.inputAttachments.push_back(ALBEDO_ATTACHMENT_INDEX);
	subpassGather.inputAttachments.push_back(NORMAL_ATTACHMENT_INDEX);
	subpassGather.inputAttachments.push_back(DEPTH_ATTACHMENT_INDEX);
	subpassGather.inputAttachments.push_back(SHADOWMAP_DEPTH_ATTACHMENT_0_INDEX);
	subpassGather.inputAttachments.push_back(SHADOWMAP_DEPTH_ATTACHMENT_1_INDEX);
	subpassGather.inputAttachments.push_back(SHADOWMAP_DEPTH_ATTACHMENT_2_INDEX);

	subpassGather.pipelineType = GFX::PipelineType::Graphics;

	// Subpass 5 Present
	GFX::SubPassDescription subpassPresent = {};
	// Render To Swapchain
	subpassPresent.colorAttachments.push_back(0);
	//// HDR Input
	subpassPresent.inputAttachments.push_back(HDR_ATTACHMENT_INDEX);
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

	GFX::DependencyDescription dependencyDesc2 = {};
	dependencyDesc2.srcSubpass = 2;
	dependencyDesc2.dstSubpass = 3;
	dependencyDesc2.srcStage = GFX::PipelineStage::ColorAttachmentOutput;
	dependencyDesc2.dstStage = GFX::PipelineStage::FragmentShader;
	dependencyDesc2.srcAccess = GFX::Access::ColorAttachmentWrite;
	dependencyDesc2.dstAccess = GFX::Access::ShaderRead;

	GFX::DependencyDescription dependencyDesc3 = {};
	dependencyDesc3.srcSubpass = 3;
	dependencyDesc3.dstSubpass = 4;
	dependencyDesc3.srcStage = GFX::PipelineStage::ColorAttachmentOutput;
	dependencyDesc3.dstStage = GFX::PipelineStage::FragmentShader;
	dependencyDesc3.srcAccess = GFX::Access::ColorAttachmentWrite;
	dependencyDesc3.dstAccess = GFX::Access::ShaderRead;

	GFX::DependencyDescription dependencyDesc4 = {};
	dependencyDesc4.srcSubpass = 4;
	dependencyDesc4.dstSubpass = 5;
	dependencyDesc4.srcStage = GFX::PipelineStage::ColorAttachmentOutput;
	dependencyDesc4.dstStage = GFX::PipelineStage::FragmentShader;
	dependencyDesc4.srcAccess = GFX::Access::ColorAttachmentWrite;
	dependencyDesc4.dstAccess = GFX::Access::ShaderRead;

	GFX::RenderPassDescription renderPassDesc = {};
	renderPassDesc.width = s_width;
	renderPassDesc.height = s_height;

	renderPassDesc.attachments.push_back(swapChainAttachment);
	renderPassDesc.attachments.push_back(gBufferAlbedoAttachment);
	renderPassDesc.attachments.push_back(gBufferNormalRoughnessAttachment);
	renderPassDesc.attachments.push_back(hdrAttachment);
	renderPassDesc.attachments.push_back(depthAttachment);
	renderPassDesc.attachments.push_back(shadowMapDepthAttachment0);
	renderPassDesc.attachments.push_back(shadowMapDepthAttachment1);
	renderPassDesc.attachments.push_back(shadowMapDepthAttachment2);

	renderPassDesc.subpasses.push_back(subpassShadowMap0);
	renderPassDesc.subpasses.push_back(subpassShadowMap1);
	renderPassDesc.subpasses.push_back(subpassShadowMap2);
	renderPassDesc.subpasses.push_back(subpassGBuffer);
	renderPassDesc.subpasses.push_back(subpassGather);
	renderPassDesc.subpasses.push_back(subpassPresent);

	renderPassDesc.dependencies.push_back(dependencyDesc0);
	renderPassDesc.dependencies.push_back(dependencyDesc1);
	renderPassDesc.dependencies.push_back(dependencyDesc2);
	renderPassDesc.dependencies.push_back(dependencyDesc3);
	renderPassDesc.dependencies.push_back(dependencyDesc4);

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
	s_meshPipelineObject->Build(s_meshRenderPass, MRT_PASS_INDEX, 1, vertexBindings, uniformBindings, "screen-space-reflection/default.vert", "screen-space-reflection/default.frag", true);
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
	s_meshMRTPipelineObject->Build(s_meshRenderPass, MRT_PASS_INDEX, 2, vertexBindings, uniformBindings, "screen-space-reflection/default.vert", "screen-space-reflection/defaultMRT.frag", true);
}

void CreateGatheringPipeline()
{
	// Uniform Buffer
	GFX::BufferDescription gatherUniformBufferDesc = {};
	gatherUniformBufferDesc.size = GFX::UniformAlign(sizeof(GatheringPassUniformData));
	gatherUniformBufferDesc.storageMode = GFX::BufferStorageMode::Dynamic;
	gatherUniformBufferDesc.usage = GFX::BufferUsage::UniformBuffer;

	s_gatheringPassUniformBuffer = GFX::CreateBuffer(gatherUniformBufferDesc);

	// Uniform Layout
	GFX::UniformLayoutDescription uniformLayoutDescription = {};
	uniformLayoutDescription.AddUniformBinding(0, GFX::UniformType::InputAttachment, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(1, GFX::UniformType::InputAttachment, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(2, GFX::UniformType::UniformBuffer, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(3, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(4, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(5, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(6, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(7, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(8, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);

    s_gatherUniformLayout =	GFX::CreateUniformLayout(uniformLayoutDescription);

	// Uniform
	GFX::UniformDescription uniformDesc = {};
	uniformDesc.AddInputAttachmentAttribute(0, s_meshRenderPass, ALBEDO_ATTACHMENT_INDEX);
	uniformDesc.AddInputAttachmentAttribute(1, s_meshRenderPass, NORMAL_ATTACHMENT_INDEX);
	uniformDesc.AddBufferAttribute(2, s_gatheringPassUniformBuffer, 0, GFX::UniformAlign(sizeof(GatheringPassUniformData)));
	uniformDesc.AddImageAttribute(3, skybox->image, skybox->sampler);
	uniformDesc.AddImageAttribute(4, s_irradianceMap, skybox->sampler);
	uniformDesc.AddSampledAttachmentAttribute(5, s_meshRenderPass, DEPTH_ATTACHMENT_INDEX, s_nearestSampler);
	uniformDesc.AddSampledAttachmentAttribute(6, s_meshRenderPass, SHADOWMAP_DEPTH_ATTACHMENT_0_INDEX, s_nearestSampler);
	uniformDesc.AddSampledAttachmentAttribute(7, s_meshRenderPass, SHADOWMAP_DEPTH_ATTACHMENT_1_INDEX, s_nearestSampler);
	uniformDesc.AddSampledAttachmentAttribute(8, s_meshRenderPass, SHADOWMAP_DEPTH_ATTACHMENT_2_INDEX, s_nearestSampler);
	uniformDesc.SetUniformLayout(s_gatherUniformLayout);
	uniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);

	s_gatherUniform = GFX::CreateUniform(uniformDesc);

	GFX::VertexBindings vertexBindings = {};

	GFX::UniformBindings uniformBindings = {};
	uniformBindings.AddUniformLayout(s_gatherUniformLayout);
	uniformBindings.AddUniformLayout(s_shadowMap->uniformLayout);
	uniformBindings.AddUniformLayout(s_shadowMap->uniformLayout);
	uniformBindings.AddUniformLayout(s_shadowMap->uniformLayout);

	s_gatherPipelineObject = new PipelineObject();
	s_gatherPipelineObject->Build(s_meshRenderPass, GATHER_PASS_INDEX, 1, vertexBindings, uniformBindings, "screen-space-reflection/screen_quad.vert", "screen-space-reflection/gather_pass.frag", false, GFX::CullFace::None);
}

void CreatePresentPipeline()
{
	// Uniform Layout
	GFX::UniformLayoutDescription uniformLayoutDescription = {};
	// uniformLayoutDescription.AddUniformBinding(0, GFX::UniformType::InputAttachment, GFX::ShaderStage::Fragment, 1);
	uniformLayoutDescription.AddUniformBinding(0, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);

	s_presentUniformLayout = GFX::CreateUniformLayout(uniformLayoutDescription);

	// Uniform
	GFX::UniformDescription uniformDesc = {};
	// HDR attachment 
	// uniformDesc.AddInputAttachmentAttribute(0, s_meshRenderPass, 4);
	uniformDesc.AddSampledAttachmentAttribute(0, s_meshRenderPass, HDR_ATTACHMENT_INDEX, s_nearestSampler);
	uniformDesc.SetUniformLayout(s_presentUniformLayout);
	uniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);

	s_presentUniform = GFX::CreateUniform(uniformDesc);

	GFX::VertexBindings vertexBindings = {};

	GFX::UniformBindings uniformBindings = {};
	uniformBindings.AddUniformLayout(s_presentUniformLayout);

	s_presentPipelineObject = new PipelineObject();
	s_presentPipelineObject->Build(s_meshRenderPass, PRESENT_PASS_INDEX, 1, vertexBindings, uniformBindings, "screen-space-reflection/screen_quad.vert", "screen-space-reflection/present_pass.frag", false, GFX::CullFace::None);
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
	
	s_modelUniform = CreateModelUniformBlock("screen-space-reflection/texture.jpg", "screen-space-reflection/white.jpg");
	s_waterUniform = CreateModelUniformBlock("screen-space-reflection/white.jpg", "screen-space-reflection/black.tga");

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
	
	s_shadowMap = ShadowMap::Create(s_meshRenderPass, s_width, s_height);

	skybox = Skybox::Create();

	std::vector<std::string> textureNames;
	textureNames.reserve(6);

	textureNames.push_back("screen-space-reflection/skybox/radiance_posz.tga");
	textureNames.push_back("screen-space-reflection/skybox/radiance_negz.tga");
	textureNames.push_back("screen-space-reflection/skybox/radiance_posy.tga");
	textureNames.push_back("screen-space-reflection/skybox/radiance_negy.tga");
	textureNames.push_back("screen-space-reflection/skybox/radiance_posx.tga");
	textureNames.push_back("screen-space-reflection/skybox/radiance_negx.tga");

	s_irradianceMap = Skybox::LoadCubeMap(textureNames);

	// CreateMeshPipeline();
	CreateMeshMRTPipeline();
	CreateGatheringPipeline();
	CreatePresentPipeline();

	s_camera = new Camera();
}

void ScreenSpaceReflectionExample::MainLoop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
		{
			s_camera->ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
		}
		if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
		{
			s_camera->ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
		}
		if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
		{
			s_camera->ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
		}
		if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
		{
			s_camera->ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
		}

		s_camera->UpdateAspect(s_width, s_height);

		if (GFX::BeginFrame())
		{
			GFX::BeginRenderPass(s_meshRenderPass, 0, 0, s_width, s_height);

			GFX::SetViewport(0, 0, s_width, s_height);
			GFX::SetScissor(0, 0, s_width, s_height);
			
			glm::vec4 lightDir = glm::vec4(glm::normalize(glm::vec3(100.463f, -26.725f, 0.0f)), 0.0f);

			// glm::vec3 forward = glm::inverse(s_camera->GetViewMatrix()) * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);

			// Shadow Map Rendering
			s_shadowMap->Render(s_scene, target, s_camera, (float)s_width/(float)s_height, lightDir);

			GFX::NextSubpass();

			UniformBufferObject ubo = {};
			ubo.view = s_camera->GetViewMatrix();
			ubo.proj = s_camera->GetProjectionMatrix();

			GatheringPassUniformData gatherPassUBO = {};
			gatherPassUBO.view = s_camera->GetViewMatrix();
			gatherPassUBO.proj = s_camera->GetProjectionMatrix();
			gatherPassUBO.lightDir = lightDir;
			gatherPassUBO.lightColor = glm::vec4(0.7f, 0.4f, 0.5f, 1.0f);

			GFX::UpdateUniformBuffer(s_modelUniform->uniform, 0, &ubo);
			GFX::UpdateUniformBuffer(s_waterUniform->uniform, 0, &ubo);
			GFX::UpdateUniformBuffer(s_gatherUniform, 2, &gatherPassUBO);

			GFX::ApplyPipeline(s_meshMRTPipelineObject->pipeline);


			GFX::BindUniform(s_waterUniform->uniform, 0);
			auto water = s_scene->meshes[1];
			GFX::BindIndexBuffer(water->indexBuffer, 0, GFX::IndexType::UInt32);
			GFX::BindVertexBuffer(water->vertexBuffer, 0);
			GFX::DrawIndexed(water->indices.size(), 1, 0);
				

			GFX::BindUniform(s_modelUniform->uniform, 0);
			for (int i = 0; i < s_scene->meshes.size(); i++)
			{
				if (i == 1)
				{
					continue;
				}

				auto mesh = s_scene->meshes[i];
				GFX::BindIndexBuffer(mesh->indexBuffer, 0, GFX::IndexType::UInt32);
				GFX::BindVertexBuffer(mesh->vertexBuffer, 0);
				GFX::DrawIndexed(mesh->indices.size(), 1, 0);
			}

			GFX::NextSubpass();
			GFX::ApplyPipeline(skybox->pipeline);
			// sky box
			GFX::BindUniform(skybox->uniform, 0);
			GFX::BindVertexBuffer(skybox->vertexBuffer, 0);
			GFX::Draw(108, 1, 0, 0);

			GFX::ApplyPipeline(s_gatherPipelineObject->pipeline);
			GFX::BindUniform(s_gatherUniform, 0);
			GFX::BindUniform(s_shadowMap->uniform0, 1);
			GFX::BindUniform(s_shadowMap->uniform1, 2);
			GFX::BindUniform(s_shadowMap->uniform2, 3);
			GFX::Draw(3, 1, 0, 0);

			GFX::NextSubpass();
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
	delete s_waterUniform;

	delete s_shadowMap;
	delete s_camera;

	GFX::DestroyImage(s_irradianceMap);

	GFX::DestroyBuffer(s_gatheringPassUniformBuffer);
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