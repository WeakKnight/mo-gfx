#include "modelviewer.h"

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

}

void ModelViewerExample::MainLoop()
{

}

void ModelViewerExample::CleanUp()
{
    
}