#include "surface.h"
#include <GLFW/glfw3.h>
#include "instance.h"

Surface::Surface(std::shared_ptr<Instance> instance, GLFWwindow* window)
{
	m_window = window;
	m_instance = instance->GetApiHandle();

	VkResult result = glfwCreateWindowSurface(instance->GetApiHandle(), window, nullptr, &m_apiHandle);
	assert(result == VK_SUCCESS);
}

Surface::~Surface()
{
	vkDestroySurfaceKHR(m_instance, m_apiHandle, nullptr);
}