#include "instance.h"
#include <spdlog/spdlog.h>

Instance::Instance(std::vector<std::string> enabledExtensions, std::vector<std::string> enabledLayers)
{
	VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Mo Renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    m_extensions.resize(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_extensions.data());

    std::vector<const char*> extensionCStrings;
    extensionCStrings.reserve(enabledExtensions.size());
    for (const auto& s : enabledExtensions)
    {
        extensionCStrings.push_back(&s[0]);
    }

    createInfo.enabledExtensionCount = enabledExtensions.size();
    createInfo.ppEnabledExtensionNames = extensionCStrings.data();
    

    
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    m_availableLayers.resize(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, m_availableLayers.data());

    std::vector<const char*> layerCStrings;
    if (CheckLayers(enabledLayers))
    {
        layerCStrings.reserve(enabledLayers.size());
        for (const auto& s : enabledLayers)
        {
            layerCStrings.push_back(&s[0]);
        }

        createInfo.enabledLayerCount = enabledLayers.size();
        createInfo.ppEnabledLayerNames = layerCStrings.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }
    
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    assert(result == VK_SUCCESS);

    PickPhysicalDevice();
}

Instance::~Instance()
{
    if (m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_instance, nullptr);
    }
}

bool Instance::CheckLayers(const std::vector<std::string> requestLayers) const
{
    for (auto& layerName : requestLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : m_availableLayers) {
            if (strcmp(layerName.c_str(), layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void Instance::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    assert(deviceCount != 0);
    m_physicalDevices.resize(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, m_physicalDevices.data());

    for (const auto& physicalDevice : m_physicalDevices)
    {
        if (IsDeviceSuitable(physicalDevice))
        {
            m_physicalDevice = physicalDevice;
            break;
        }
    }

    assert(m_physicalDevice != VK_NULL_HANDLE);
}

bool Instance::IsDeviceSuitable(VkPhysicalDevice device) const
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}