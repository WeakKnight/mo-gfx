#pragma once
#include "common.h"

struct GLFWwindow;

class App
{
public:
	void Run() 
	{
		Init();

		MainLoop();

		CleanUp();
	}

private:
	virtual void Init()
	{
	}

	virtual void MainLoop()
	{
	}

	virtual void CleanUp()
	{
	}

protected:
	GLFWwindow* m_window = nullptr;
};

