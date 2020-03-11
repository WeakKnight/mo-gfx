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

	void LoadTexture();

private:
	GLFWwindow* m_window = nullptr;
};