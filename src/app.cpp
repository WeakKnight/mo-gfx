#include "app.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

const int WIDTH = 800;
const int HEIGHT = 600;

std::vector<const char*> requiredExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

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

	m_instance = std::make_shared<Instance>(extensions, validationLayers);
	m_surface = std::make_shared<Surface>(m_instance, m_window);
	m_device = std::make_shared<Device>(m_instance->GetPhysicalDevice(), m_surface, requiredExtensions);
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