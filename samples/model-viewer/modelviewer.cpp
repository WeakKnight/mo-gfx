#include "modelviewer.h"
#include "spdlog/spdlog.h"
#include <GLFW/glfw3.h>
#include "gfx.h"

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>

const int WIDTH = 800;
const int HEIGHT = 600;

static int s_width = WIDTH;
static int s_height = HEIGHT;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	s_width = width;
	s_height = height;

	spdlog::info("WINDOW Resize");

	// spdlog::info("Window Resize");
	GFX::Resize(width, height);
	// GFX::ResizeRenderPass(renderPass, width, height);
}

class Model
{

	std::vector<float> vertices;
	std::vector<uint32_t> indices;
};

int main(int, char** args)
{
	InitEnvironment(args);

	App* app = new ModelViewerExample();
	app->Run();

	delete app;

	return 0;
}

void ModelViewerExample::Init()
{
	spdlog::info("Hello Model Viewer");

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "mo-gfx", nullptr, nullptr);

	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void ModelViewerExample::MainLoop()
{

}

void ModelViewerExample::CleanUp()
{
    
}