#include "app.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

const int WIDTH = 800;
const int HEIGHT = 600;

void App::Run()
{
	InitWindow();
	InitVulkan();

	MainLoop();

	CleanUp();
}

void App::InitWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	
	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void App::InitVulkan()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<std::string> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	const std::vector<std::string> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

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
	glfwDestroyWindow(m_window);
	glfwTerminate();
}