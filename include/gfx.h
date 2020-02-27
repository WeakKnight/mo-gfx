#pragma once
#include "common.h"

struct GLFWwindow;

namespace GFX
{
    struct InitialDescription
    {
        bool debugMode = false;
        GLFWwindow* window = nullptr;
    };

    void Init(const InitialDescription& desc);

    void Submit();

    void Shutdown();
};