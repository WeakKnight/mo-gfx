#pragma once
#include <vulkan/vulkan.hpp>
#include "common.h"

struct GLFWwindow;
class Instance;

class Surface
{
public:
	Surface(std::shared_ptr<Instance> instance, GLFWwindow* window);
	~Surface();

	VkSurfaceKHR GetApiHandle() const
	{
		return m_apiHandle;
	}

private:
	GLFWwindow* m_window = nullptr;
	VkSurfaceKHR m_apiHandle = VK_NULL_HANDLE;
	VkInstance m_instance = VK_NULL_HANDLE;
};