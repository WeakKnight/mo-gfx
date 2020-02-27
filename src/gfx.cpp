#include "gfx.h"
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.hpp>
#include <spdlog/spdlog.h>

#define VK_ASSERT(resultObj) assert(resultObj.result == vk::Result::eSuccess)

namespace GFX
{
    /*
    ===========================================Internal Struct Declaration========================================================
    */

    struct PipelineResource;
    struct ShaderResource;

    /*
    ===================================================Static Global Variables====================================================
    */

    /*
    Handle Pools
    */
    static HandlePool<PipelineResource> s_pipelineHandlePool = HandlePool<PipelineResource>(200);
    static HandlePool<ShaderResource> s_shaderHandlePool = HandlePool<ShaderResource>(200);

    /*
    Device Instance
    */
    static vk::Instance s_instance = nullptr;
    static vk::PhysicalDevice s_physicalDevice = nullptr;
    static vk::Device s_device = nullptr;

    static VkSurfaceKHR s_surface = nullptr;

    /*
    Supported Extensions And Layers
    */
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

    static SwapChainSupportDetails s_swapChainSupportDetails;
    static vk::SwapchainKHR s_swapChain = nullptr;
    std::vector<VkImage> s_swapChainImages;
    vk::Format s_swapChainImageFormat;
    vk::Extent2D s_swapChainImageExtent;
    std::vector<VkImageView> s_swapChainImageViews;

    /*
    Extension And Layer Info
    */
    static std::vector<const char*> s_expectedLayers =
    {
    };

    static std::vector<const char*> s_expectedExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    /*
    ===========================================Internal Struct Definition===================================================
    */

    struct ShaderResource
    {
        ShaderResource(const ShaderDescription& desc)
        {
            shaderc_shader_kind shaderKind = MapShaderStageForShaderc(desc.stage);
            std::vector<uint32_t> spirvCodes = CompileFile(desc.name, shaderKind, desc.codes, false);

            vk::ShaderModuleCreateInfo createInfo = {};
            createInfo.setCodeSize(spirvCodes.size() * sizeof(uint32_t));
            createInfo.setPCode(spirvCodes.data());

            auto createShaderModuleResult = s_device.createShaderModule(createInfo);
            VK_ASSERT(createShaderModuleResult);
            m_shaderModule = createShaderModuleResult.value;

            m_shaderStage = desc.stage;
        }

        ~ShaderResource()
        {
            s_device.destroyShaderModule(m_shaderModule);
        }

        shaderc_shader_kind MapShaderStageForShaderc(const ShaderStage& stage)
        {
            switch (stage)
            {
            case ShaderStage::Vertex:
                return shaderc_shader_kind::shaderc_vertex_shader;
            case ShaderStage::Compute:
                return shaderc_shader_kind::shaderc_compute_shader;
            case ShaderStage::Fragment:
                return shaderc_shader_kind::shaderc_fragment_shader;
            default:
                return shaderc_shader_kind::shaderc_glsl_infer_from_source;
            }
        }

        vk::ShaderStageFlagBits MapShaderStageForVulkan(const ShaderStage& stage)
        {
            switch (stage)
            {
            case ShaderStage::Vertex:
                return vk::ShaderStageFlagBits::eVertex;
            case ShaderStage::Compute:
                return vk::ShaderStageFlagBits::eCompute;
            case ShaderStage::Fragment:
                return vk::ShaderStageFlagBits::eFragment;
            default:
                return vk::ShaderStageFlagBits::eAll;
            }
        }

        vk::PipelineShaderStageCreateInfo GetShaderStageCreateInfo()
        {
            vk::PipelineShaderStageCreateInfo shaderStageCreateInfo = {};
            shaderStageCreateInfo.setStage(MapShaderStageForVulkan(m_shaderStage));
            shaderStageCreateInfo.setModule(m_shaderModule);
            shaderStageCreateInfo.setPName("main");

            return shaderStageCreateInfo;
        }

        std::vector<uint32_t> CompileFile(const std::string& sourceName,
            shaderc_shader_kind kind,
            const std::string& source,
            bool optimize)
        {
            shaderc::Compiler compiler;
            shaderc::CompileOptions options;

            // Like -DMY_DEFINE=1
            options.AddMacroDefinition("MY_DEFINE", "1");
            if (optimize)
                options.SetOptimizationLevel(shaderc_optimization_level_size);

            shaderc::SpvCompilationResult module =
                compiler.CompileGlslToSpv(source, kind, sourceName.c_str(), options);

            auto status = module.GetCompilationStatus();
            if (status != shaderc_compilation_status_success)
            {
                std::string errorMsg = module.GetErrorMessage();
                spdlog::error("Shader Compiling Failed. Error: {}", errorMsg);
                if (status == shaderc_compilation_status_invalid_stage)
                {
                    spdlog::error("Wrong Shader Stage");
                }
                return std::vector<uint32_t>();
            }

            return { module.cbegin(), module.cend() };
        }

        uint32_t handle = 0;
        vk::ShaderModule m_shaderModule = nullptr;
        ShaderStage m_shaderStage = ShaderStage::None;
    };

    struct PipelineResource
    {
        PipelineResource(const PipelineDescription& desc)
        {
            std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos = {};
            for (auto shader : desc.shaders)
            {
                ShaderResource* shaderResource = s_shaderHandlePool.FetchResource(shader.id);
                shaderStageCreateInfos.push_back(shaderResource->GetShaderStageCreateInfo());
            }

            vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
            vertexInputStateCreateInfo.setVertexBindingDescriptionCount(0);
            vertexInputStateCreateInfo.setVertexAttributeDescriptionCount(0);

            vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
            inputAssemblyStateCreateInfo.setTopology(MapPrimitiveTopologyForVulkan(desc.primitiveTopology));
            inputAssemblyStateCreateInfo.setPrimitiveRestartEnable(false);

            vk::Viewport viewPort = {};
            viewPort.setX(0.0f);
            viewPort.setY(0.0f);
            viewPort.setWidth(s_swapChainImageExtent.width);
            viewPort.setHeight(s_swapChainImageExtent.height);
            viewPort.setMinDepth(0.0f);
            viewPort.setMaxDepth(1.0f);

            vk::Rect2D scissor = {};
            scissor.setOffset({ 0, 0 });
            scissor.setExtent(s_swapChainImageExtent);

            vk::PipelineViewportStateCreateInfo viewportStateCreateInfo = {};
            viewportStateCreateInfo.setViewportCount(1);
            viewportStateCreateInfo.setPViewports(&viewPort);
            viewportStateCreateInfo.setScissorCount(1);
            viewportStateCreateInfo.setPScissors(&scissor);

            vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
            rasterizationStateCreateInfo.setDepthClampEnable(false);
            rasterizationStateCreateInfo.setRasterizerDiscardEnable(false);
            rasterizationStateCreateInfo.setPolygonMode(vk::PolygonMode::eFill);
            rasterizationStateCreateInfo.setLineWidth(1.0f);
            rasterizationStateCreateInfo.setCullMode(vk::CullModeFlagBits::eBack);
            rasterizationStateCreateInfo.setFrontFace(vk::FrontFace::eClockwise);
            rasterizationStateCreateInfo.setDepthBiasEnable(false);

            vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
            multisampleStateCreateInfo.setSampleShadingEnable(false);
            multisampleStateCreateInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);

            vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = {};
            colorBlendAttachmentState.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
            colorBlendAttachmentState.setBlendEnable(false);

            vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
            colorBlendStateCreateInfo.setLogicOpEnable(false);
            colorBlendStateCreateInfo.setAttachmentCount(1);
            colorBlendStateCreateInfo.setPAttachments(&colorBlendAttachmentState);

            std::vector<vk::DynamicState> dynamicStates =
            {
                vk::DynamicState::eViewport,
            };

            vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
            dynamicStateCreateInfo.setDynamicStateCount(dynamicStates.size());
            dynamicStateCreateInfo.setPDynamicStates(dynamicStates.data());
            
            vk::PipelineLayoutCreateInfo layoutCreateInfo = {};
            
            
        }

        ~PipelineResource()
        {
            s_device.destroyPipeline(m_pipeline);
        }

        vk::PrimitiveTopology MapPrimitiveTopologyForVulkan(const PrimitiveTopology& primitiveTopology)
        {
            switch (primitiveTopology)
            {
            case PrimitiveTopology::PointList:
                return vk::PrimitiveTopology::ePointList;
            case PrimitiveTopology::LineList:
                return vk::PrimitiveTopology::eLineList;
            case PrimitiveTopology::LineStrip:
                return vk::PrimitiveTopology::eLineStrip;
            case PrimitiveTopology::TriangleList:
                return vk::PrimitiveTopology::eTriangleList;
            case PrimitiveTopology::TriangleStrip:
                return vk::PrimitiveTopology::eTriangleStrip;
            default:
                assert(false);
                return vk::PrimitiveTopology::eTriangleList;
            }
        }

        uint32_t handle = 0;
        vk::Pipeline m_pipeline = nullptr;
    };

    /*
    =============================================Internal Interface Declaration====================================================
    */

    bool CheckLayerSupport(const std::vector<const char*> expectedLayers);

    vk::PhysicalDevice ChooseDevice(const std::vector<vk::PhysicalDevice>& physicalDevices);

    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

    void CreateSwapChain();
    void CreateImageViews();

    /*
    =================================================Implementation===========================================================
    */

    /*
    Resource Management
    */

    Pipeline CreatePipeline(const PipelineDescription& desc)
    {
        Pipeline result = Pipeline();
        
        PipelineResource* pipelineResource = new PipelineResource(desc);
        result.id = s_pipelineHandlePool.AllocateHandle(pipelineResource);
        
        pipelineResource->handle = result.id;

        return result;
    }

    Shader CreateShader(const ShaderDescription& desc)
    {
        Shader result = Shader();

        ShaderResource* shaderResource = new ShaderResource(desc);
        result.id = s_shaderHandlePool.AllocateHandle(shaderResource);

        shaderResource->handle = result.id;

        return result;
    }

    void DestroyShader(const Shader& shader)
    {
        s_shaderHandlePool.FreeHandle(shader.id);
    }

    /*
    Life Cycle
    */

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

        CreateSwapChain();
        CreateImageViews();
    }

    void Submit()
    {

    }

    void Frame()
    {

    }

    void Shutdown()
    {
        for (auto imageView : s_swapChainImageViews)
        {
            vkDestroyImageView(s_device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(s_device, s_swapChain, nullptr);
        vkDestroySurfaceKHR(s_instance, s_surface, nullptr);
        s_device.destroy();
        s_instance.destroy();
    }

    void CreateImageViews()
    {
        for (size_t i = 0; i < s_swapChainImages.size(); i++)
        {
            vk::ImageViewCreateInfo createInfo = {};
            createInfo.setImage(s_swapChainImages[i]);
            createInfo.setViewType(vk::ImageViewType::e2D);
            createInfo.setFormat(s_swapChainImageFormat);

            vk::ComponentMapping componentMapping = {};

            createInfo.setComponents(componentMapping);

            vk::ImageSubresourceRange subResourceRange = {};
            subResourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
            subResourceRange.setBaseMipLevel(0);
            subResourceRange.setLevelCount(1);
            subResourceRange.setBaseArrayLayer(0);
            subResourceRange.setLayerCount(1);

            createInfo.setSubresourceRange(subResourceRange);

            auto createImageViewResult = s_device.createImageView(createInfo);
            VK_ASSERT(createImageViewResult);
            s_swapChainImageViews.push_back(createImageViewResult.value);
        }
    }

    void CreateSwapChain()
    {
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

        vk::SurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(s_swapChainSupportDetails.formats);
        vk::PresentModeKHR presentMode = ChoosePresentMode(s_swapChainSupportDetails.presentModes);
        vk::Extent2D extent = ChooseSwapExtent(s_swapChainSupportDetails.capabilities);

        uint32_t imageCount = s_swapChainSupportDetails.capabilities.minImageCount + 1;
        if (s_swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > s_swapChainSupportDetails.capabilities.maxImageCount)
        {
            imageCount = s_swapChainSupportDetails.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo = {};
        createInfo.setSurface(s_surface);
        createInfo.setMinImageCount(imageCount);
        createInfo.setImageFormat(surfaceFormat.format);
        createInfo.setImageColorSpace(surfaceFormat.colorSpace);
        createInfo.setImageExtent(extent);
        createInfo.setImageArrayLayers(1);
        createInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

        uint32_t queueFamilyIndices[] = { s_graphicsFamily, s_presentFamily };

        if (s_graphicsFamily != s_presentFamily)
        {
            createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
            createInfo.setQueueFamilyIndexCount(2);
            createInfo.setPQueueFamilyIndices(queueFamilyIndices);
        }
        else
        {
            createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        }

        createInfo.setPreTransform(s_swapChainSupportDetails.capabilities.currentTransform);
        createInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        createInfo.setPresentMode(presentMode);
        createInfo.setClipped(true);
        createInfo.setOldSwapchain(nullptr);

        auto createSwapChainResult = s_device.createSwapchainKHR(createInfo);
        VK_ASSERT(createSwapChainResult);
        s_swapChain = createSwapChainResult.value;

        vkGetSwapchainImagesKHR(s_device, s_swapChain, &imageCount, nullptr);
        s_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(s_device, s_swapChain, &imageCount, s_swapChainImages.data());

        s_swapChainImageFormat = surfaceFormat.format;
        s_swapChainImageExtent = extent;
    }

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

    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        for (auto format : availableFormats)
        {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                return format;
            }
        }

        return availableFormats[0];
    }

    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
        for (auto presentMode : availablePresentModes)
        {
            if (presentMode == vk::PresentModeKHR::eMailbox)
            {
                return presentMode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        assert(capabilities.currentExtent.width != UINT32_MAX);
        return capabilities.currentExtent;
    }
};