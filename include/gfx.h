#pragma once
#include "common.h"


namespace GFX
{
    struct InitialDescription
    {
        bool debugMode = false;
    };

    void Init(const InitialDescription& desc);

    void Draw();

    void Shutdown();
};