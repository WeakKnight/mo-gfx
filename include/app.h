#pragma once
#include "common.h"

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
};