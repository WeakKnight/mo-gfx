#pragma once
#include "common.h"
#include "gpu.h"

struct GLFWwindow;

class App
{
public:
	void Run();

private:
	void InitWindow();
	void InitVulkan();
	void MainLoop();
	void CleanUp();

private:
	GLFWwindow* m_window = nullptr;
	std::shared_ptr<Instance> m_instance;
	std::shared_ptr<Device> m_device;
	std::shared_ptr<Surface> m_surface;
	std::shared_ptr<SwapChain> m_swapChain;
};