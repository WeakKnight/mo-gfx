#include "modelviewer.h"
#include "spdlog/spdlog.h"

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
}

void ModelViewerExample::MainLoop()
{

}

void ModelViewerExample::CleanUp()
{
    
}