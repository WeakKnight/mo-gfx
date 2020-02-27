#include "gfx.h"
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define VK_ASSERT(resultObj) assert(resultObj.result == vk::Result::eSuccess)

namespace GFX
{
    struct SwapChainSupportDetails 
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;

        bool IsAdequate()
        {
            return (formats.size() != 0 && presentModes.size() != 0);
        }
    };

    static vk::Instance s_instance = nullptr;
    static vk::PhysicalDevice s_physicalDevice = nullptr;
    static vk::Device s_device = nullptr;

    static VkSurfaceKHR s_surface = nullptr;

    static std::vector<vk::ExtensionProperties> s_extensions;
    static std::vector<vk::LayerProperties> s_layers;

    /*
    Queue Family
    */
    static uint32_t s_graphicsFamily;
    static uint32_t s_presentFamily;
    /*
    Default Queue
    */
    static vk::Queue s_graphicsQueueDefault = nullptr;
    static vk::Queue s_presentQueueDefault = nullptr;
    /*
    Swap Chain
    */
    static SwapChainSupportDetails s_swapChainSupportDetails;

    static std::vector<const char*> s_expectedLayers = 
    {
    };

    static std::vector<const char*> s_expectedExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    bool CheckLayerSupport(const std::vector<const char*> expectedLayers)
    {
        for (const char* layerName : expectedLayers) 
        {
            bool layerFound = false;

            for (const auto& layerProperties : s_layers) 
            {
                if (strcmp(layerName, layerProperties.layerName) == 0) 
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) 
            {
                return false;
            }
        }

        return true;
    }

    vk::PhysicalDevice ChooseDevice(const std::vector<vk::PhysicalDevice>& physicalDevices)
    {
        for (const auto physicalDevice : physicalDevices)
        {
            if (physicalDevice.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                return physicalDevice;
            }
        }

        assert(physicalDevices.size() >= 1);
        return physicalDevices[0];
    }

    void Init(const InitialDescription& desc)
    {
        auto enumerateInstanceExtensionPropsResult = vk::enumerateInstanceExtensionProperties();
        VK_ASSERT(enumerateInstanceExtensionPropsResult);
        s_extensions = enumerateInstanceExtensionPropsResult.value;

        auto enumerateInstanceLayerPropsResult = vk::enumerateInstanceLayerProperties();
        VK_ASSERT(enumerateInstanceLayerPropsResult);
        s_layers = enumerateInstanceLayerPropsResult.value;

        vk::ApplicationInfo appInfo = vk::ApplicationInfo();
        appInfo.setPApplicationName("Mo Renderer");
        appInfo.setApplicationVersion(VK_MAKE_VERSION(0, 0, 1));
        appInfo.setPEngineName("MO");
        appInfo.setEngineVersion(VK_MAKE_VERSION(0, 0, 1));
        appInfo.setApiVersion(VK_API_VERSION_1_2);

        vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo();
        createInfo.setPApplicationInfo(&appInfo);

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        createInfo.setEnabledExtensionCount(glfwExtensionCount);
        createInfo.setPpEnabledExtensionNames(glfwExtensions);

        if (desc.debugMode)
        {
            s_expectedLayers.push_back("VK_LAYER_KHRONOS_validation");   
        }

        if (CheckLayerSupport(s_expectedLayers))
        {
            createInfo.setEnabledLayerCount(s_expectedLayers.size());
            createInfo.setPpEnabledLayerNames(s_expectedLayers.data());
        }
        else
        {
            assert(false);
            createInfo.setEnabledLayerCount(0);
        }

        auto createInstanceResult = vk::createInstance(createInfo);
        VK_ASSERT(createInstanceResult);
        s_instance  = createInstanceResult.value;

        auto enumeratePhysicalDevicesResult = s_instance.enumeratePhysicalDevices();
        VK_ASSERT(enumeratePhysicalDevicesResult);
        s_physicalDevice = ChooseDevice(enumeratePhysicalDevicesResult.value);

        // Create Surface
        glfwCreateWindowSurface(s_instance, desc.window, nullptr, &s_surface);

        /*
        Create Queue Family
        */
        auto familyProperties = s_physicalDevice.getQueueFamilyProperties();
        for (uint32_t i = 0; i < familyProperties.size(); i++)
        {
            auto familyProp = familyProperties[i];
            if (familyProp.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                s_graphicsFamily = i;
            }

            auto getSurfaceSupportKHRResult = s_physicalDevice.getSurfaceSupportKHR(i, s_surface);
            VK_ASSERT(getSurfaceSupportKHRResult);
            if (getSurfaceSupportKHRResult.value)
            {
                s_presentFamily = i;
            }
        }

        /*
        Create Logical Device
        */

        // Graphics Queue Create Info
        vk::DeviceQueueCreateInfo graphicsQueueCreateInfo = {};
        graphicsQueueCreateInfo.setQueueFamilyIndex(s_graphicsFamily);
        graphicsQueueCreateInfo.setQueueCount(1);
        float graphicsQueuePriority = 1.0f;
        graphicsQueueCreateInfo.setPQueuePriorities(&graphicsQueuePriority);

        // Presemt Queue Create Info
        vk::DeviceQueueCreateInfo presentQueueCreateInfo = {};
        presentQueueCreateInfo.setQueueFamilyIndex(s_presentFamily);
        presentQueueCreateInfo.setQueueCount(1);
        float presentQueuePriority = 1.0f;
        presentQueueCreateInfo.setPQueuePriorities(&presentQueuePriority);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = { graphicsQueueCreateInfo, presentQueueCreateInfo };

        // Feature
        vk::PhysicalDeviceFeatures deviceFeatures = s_physicalDevice.getFeatures();

        // Device Create Info
        vk::DeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.setQueueCreateInfoCount(queueCreateInfos.size());
        deviceCreateInfo.setPQueueCreateInfos(queueCreateInfos.data());
        deviceCreateInfo.setEnabledExtensionCount(s_expectedExtensions.size());
        deviceCreateInfo.setPpEnabledExtensionNames(s_expectedExtensions.data());
        deviceCreateInfo.setPEnabledFeatures(&deviceFeatures);

        auto createDeviceResult = s_physicalDevice.createDevice(deviceCreateInfo);
        VK_ASSERT(createDeviceResult);
        s_device = createDeviceResult.value;

        // Create Default Queue
        s_graphicsQueueDefault = s_device.getQueue(s_graphicsFamily, 0);
        s_presentQueueDefault = s_device.getQueue(s_presentFamily, 0);

        // Query Swap Chain Info
        auto getSurfaceCapabilityiesResult = s_physicalDevice.getSurfaceCapabilitiesKHR(s_surface);
        VK_ASSERT(getSurfaceCapabilityiesResult);
        s_swapChainSupportDetails.capabilities = getSurfaceCapabilityiesResult.value;
        auto getSurfaceFormatsResult = s_physicalDevice.getSurfaceFormatsKHR(s_surface);
        VK_ASSERT(getSurfaceFormatsResult);
        s_swapChainSupportDetails.formats = getSurfaceFormatsResult.value;
        auto getSurfacePresentModesResult = s_physicalDevice.getSurfacePresentModesKHR(s_surface);
        VK_ASSERT(getSurfacePresentModesResult);
        s_swapChainSupportDetails.presentModes = getSurfacePresentModesResult.value;   
        assert(s_swapChainSupportDetails.IsAdequate());
    }

    void Submit()
    {

    }

    void Shutdown()
    {
        vkDestroySurfaceKHR(s_instance, s_surface, nullptr);
        s_device.destroy();
        s_instance.destroy();
    }
};