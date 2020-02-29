#include "app.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "gfx.h"
#include "string_utils.h"

const int WIDTH = 800;
const int HEIGHT = 600;

static GFX::Shader vertShader;
static GFX::Shader fragShader;
static GFX::Pipeline pipeline;

void App::Run()
{
	Init();

	MainLoop();

	CleanUp();
}

void App::Init()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	
	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	GFX::InitialDescription initDesc = {};
	initDesc.debugMode = true;
	initDesc.window = m_window;

	GFX::Init(initDesc);

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

	GFX::GraphicsPipelineDescription pipelineDesc = {};
	pipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
	pipelineDesc.shaders.push_back(vertShader);
	pipelineDesc.shaders.push_back(fragShader);
	pipeline = GFX::CreatePipeline(pipelineDesc);
}

void App::MainLoop()
{
	while (!glfwWindowShouldClose(m_window)) 
	{
		glfwPollEvents();

		GFX::BeginFrame();
		
		GFX::BeginDefaultRenderPass();

		GFX::SetViewport(0, 0, WIDTH, HEIGHT);
		GFX::ApplyPipeline(pipeline);
		GFX::Draw(3, 1, 0, 0);
		
		GFX::EndRenderPass();
		
		GFX::EndFrame();
	}
}

void App::CleanUp()
{
	GFX::DestroyPipeline(pipeline);

	GFX::DestroyShader(vertShader);
	GFX::DestroyShader(fragShader);

	GFX::Shutdown();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}