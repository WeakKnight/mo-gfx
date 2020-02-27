#include "app.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "gfx.h"
#include "string_utils.h"

const int WIDTH = 800;
const int HEIGHT = 600;

static GFX::Shader vertShader;
static GFX::Shader fragShader;

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
}

void App::MainLoop()
{
	while (!glfwWindowShouldClose(m_window)) 
	{
		glfwPollEvents();
	}
}

void App::CleanUp()
{
	GFX::DestroyShader(vertShader);
	GFX::DestroyShader(fragShader);

	GFX::Shutdown();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}