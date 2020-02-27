#include "app.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "gfx.h"

const int WIDTH = 800;
const int HEIGHT = 600;

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
	GFX::Shutdown();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}