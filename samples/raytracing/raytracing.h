#pragma once
#include "app.h"

class RaytracingExample : public App
{
private:
	void Init() override;

	void MainLoop() override;

	void CleanUp() override;
};

