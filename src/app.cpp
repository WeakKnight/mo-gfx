#include "app.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "gfx.h"
#include "string_utils.h"
#include <glm/glm.hpp>

const int WIDTH = 800;
const int HEIGHT = 600;

static int s_width = WIDTH;
static int s_height = HEIGHT;

static GFX::Shader vertShader;
static GFX::Shader fragShader;
static GFX::Pipeline pipeline;
static GFX::Buffer vertexBuffer;

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;
};

const std::vector<Vertex> vertices = {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

void App::Run()
{
	Init();

	MainLoop();

	CleanUp();
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) 
{
	s_width = width;
	s_height = height;
}

void App::Init()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

	GFX::InitialDescription initDesc = {};
	initDesc.debugMode = true;
	initDesc.window = m_window;

	GFX::Init(initDesc);

	GFX::BufferDescription vertexBufferDescription = {};
	vertexBufferDescription.size = sizeof(Vertex) * vertices.size();
	vertexBufferDescription.storageMode = GFX::BufferStorageMode::Static;
	vertexBufferDescription.usage = GFX::BufferUsage::VertexBuffer;

	vertexBuffer = GFX::CreateBuffer(vertexBufferDescription);
	void* data = GFX::MapBuffer(vertexBuffer, 0, vertexBufferDescription.size);
	memcpy(data, vertices.data(), vertexBufferDescription.size);
	GFX::UnmapBuffer(vertexBuffer);

	GFX::ShaderDescription vertDesc = {};
	vertDesc.name = "default";
	vertDesc.codes = StringUtils::ReadFile("default.vert");
	vertDesc.stage = GFX::ShaderStage::Vertex;

	GFX::ShaderDescription fragDesc = {};
	fragDesc.name = "default";
	fragDesc.codes = StringUtils::ReadFile("default.frag");
	fragDesc.stage = GFX::ShaderStage::Fragment;

	vertShader = GFX::CreateShader(vertDesc);
	fragShader = GFX::CreateShader(fragDesc);

	GFX::Bindings bindings = {};
	bindings.AddAttribute(0, offsetof(Vertex, pos), GFX::ValueType::Float32x2);
	bindings.AddAttribute(1, offsetof(Vertex, color), GFX::ValueType::Float32x3);
	bindings.SetStrideSize(sizeof(Vertex));
	bindings.SetBindingType(GFX::BindingType::Vertex);
	bindings.SetBindingPosition(0);

	GFX::GraphicsPipelineDescription pipelineDesc = {};
	pipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
	pipelineDesc.shaders.push_back(vertShader);
	pipelineDesc.shaders.push_back(fragShader);
	pipelineDesc.bindings = bindings;

	pipeline = GFX::CreatePipeline(pipelineDesc);
}

void App::MainLoop()
{
	while (!glfwWindowShouldClose(m_window)) 
	{
		glfwPollEvents();

		if (GFX::BeginFrame())
		{
			GFX::BeginDefaultRenderPass();

			GFX::ApplyPipeline(pipeline);

			GFX::BindVertexBuffer(vertexBuffer, 0);
			
			GFX::SetViewport(0, 0, s_width, s_height);
			GFX::SetScissor(0, 0, s_width, s_height);

			GFX::Draw(3, 1, 0, 0);

			GFX::EndRenderPass();

			GFX::EndFrame();
		}
	}
}

void App::CleanUp()
{
	GFX::DestroyBuffer(vertexBuffer);

	GFX::DestroyPipeline(pipeline);

	GFX::DestroyShader(vertShader);
	GFX::DestroyShader(fragShader);

	GFX::Shutdown();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}