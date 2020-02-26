#include "gfx.h"
#include <vulkan/vulkan.hpp>

#define VULKAN_HPP_NO_EXCEPTIONS

namespace GFX
{
 

    static vk::Instance s_instance = nullptr;
    static vk::Device s_device = nullptr;

    void Init(const InitialDescription& desc)
    {
        vk::ApplicationInfo appInfo = vk::ApplicationInfo();
        appInfo.pApplicationName = "Mo Renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.pEngineName = "MO";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo();
        createInfo.pApplicationInfo = &appInfo;

        s_instance  = vk::createInstance(createInfo);


        
    }

    void Draw()
    {

    }

    void Shutdown()
    {

    }
};