#pragma once
#include "common.h"

struct GLFWwindow;

class App
{
public:
	void Run();

private:
	void Init();
	void MainLoop();
	void CleanUp();

private:
	GLFWwindow* m_window = nullptr;
};