#include "gfx.h"
#define VULKAN_HPP_ASSERT
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.hpp>
#include <spdlog/spdlog.h>
#include <map>

#define VK_ASSERT(resultObj) assert(resultObj.result == vk::Result::eSuccess)

namespace GFX
{
    /*
    ===========================================Internal Struct Declaration========================================================
    */

    struct PipelineResource;
    struct ShaderResource;
    struct RenderPassResource;
    struct BufferResource;
    struct UniformLayoutResource;
    struct UniformResource;
    struct ImageResource;

    /*
    ===================================================Static Global Variables====================================================
    */

    /*
    Handle Pools
    */
    static HandlePool<PipelineResource> s_pipelineHandlePool = HandlePool<PipelineResource>(200);
    static HandlePool<ShaderResource> s_shaderHandlePool = HandlePool<ShaderResource>(200);
    static HandlePool<RenderPassResource> s_renderPassHandlePool = HandlePool<RenderPassResource>(200);
    static HandlePool<BufferResource> s_bufferHandlePool = HandlePool<BufferResource>(512);
    static HandlePool<UniformLayoutResource> s_uniformLayoutHandlePool = HandlePool<UniformLayoutResource>(128);
    static HandlePool<UniformResource> s_uniformHandlePool = HandlePool<UniformResource>(256);
    static HandlePool<ImageResource> s_imageHandlePool = HandlePool<ImageResource>(256);

    /*
    Device Instance
    */
    static vk::Instance s_instance = nullptr;
    static vk::PhysicalDevice s_physicalDevice = nullptr;
    static vk::PhysicalDeviceMemoryProperties s_physicalDeviceMemoryProperties;
    static vk::PhysicalDeviceProperties s_physicalDeviceProperties;
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

    uint32_t s_currentImageIndex = 0;
    uint32_t s_currentFrame = 0;

    /*
    Current Pipeline
    */
    PipelineResource* s_currentPipleline = nullptr;
    
    /*
    Default Render Pass
    */
    vk::RenderPass s_renderPassDefault = nullptr;

    /*
    Swap Chain Frame Buffers
    */
    std::vector<VkFramebuffer> s_swapChainFrameBuffers;

    /*
    Default Command Pool
    */
    vk::CommandPool s_commandPoolDefault = nullptr;

    /*
    Default Command Buffers For Swap Chain Frame Buffers
    */
    std::vector<vk::CommandBuffer> s_commandBuffersDefault;

    /*
    Default Descriptor Pool
    */
    vk::DescriptorPool s_descriptorPoolDefault = nullptr;

    /*
    Current Descriptor Set
    */
    std::map<uint32_t, vk::DescriptorSet> s_currentDescriptors;

    /*
    Sync Objects
    */
    const int MAX_FRAMES_IN_FLIGHT = 2;

    std::vector<vk::Semaphore> s_imageAvailableSemaphores;
    std::vector<vk::Semaphore> s_renderFinishedSemaphores;
    std::vector<vk::Fence> s_inFlightFences;

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
    ============================================Common Util Functions========================================================
    */
    vk::Format MapFormatForVulkan(const Format& format)
    {
        switch (format)
        {
        case Format::R32SF:
            return vk::Format::eR32Sfloat;
        case Format::R8G8B8A8:
            return vk::Format::eR8G8B8A8Unorm;
        case Format::R8G8B8:
            return vk::Format::eR8G8B8Unorm;
        case Format::R16G16B16A16F:
            return vk::Format::eR16G16B16A16Sfloat;
        case Format::R16G16B16F:
            return vk::Format::eR16G16B16Sfloat;
        case Format::R32G32B32A32F:
            return vk::Format::eR32G32B32A32Sfloat;
        case Format::R32G32B32F:
            return vk::Format::eR32G32B32Sfloat;
        default:
            assert(false);
            return vk::Format::eA1R5G5B5UnormPack16;
        }
    }

    /*
    =============================================Internal Interface Declaration====================================================
    */

    bool CheckLayerSupport(const std::vector<const char*> expectedLayers);

    vk::PhysicalDevice ChooseDevice(const std::vector<vk::PhysicalDevice>& physicalDevices);

    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

    void CreateSwapChain();
    void RecreateSwapChain();
    void CreateImageViews();
    void CreateDefaultRenderPass();
    void CreateSwapChainFramebuffers();
    void CreateCommandPoolDefault();
    void CreateCommandBuffersDefault();
    void CreateDescriptorPoolDefault();
    void CreateSyncObjects();

    vk::CommandBuffer BeginOneTimeCommandBuffer();
    void EndOneTimeCommandBuffer(vk::CommandBuffer commandBuffer);

    vk::Format MapTypeFormatForVulkan(ValueType valueType);
    vk::IndexType MapIndexTypeFormatForVulkan(IndexType indexType);
    vk::ShaderStageFlagBits MapShaderStageForVulkan(const ShaderStage& stage);
    vk::DescriptorType MapUniformTypeForVulkan(const UniformType& uniformType);
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void CreateVulkanBuffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
    void TransitionImageLayout(vk::Image img, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void CopyBufferToImage(vk::Buffer buffer, vk::Image img, uint32_t width, uint32_t height);

    uint32_t HashTwoInt(uint32_t a, uint32_t b);

    /*
    ===========================================Internal Struct Definition===================================================
    */

    struct RenderPassResource
    {
        RenderPassResource(const RenderPassDescription)
        {
            vk::AttachmentDescription colorAttachment = {};
            colorAttachment.setFormat(vk::Format::eR8G8B8A8Srgb);
        }

        ~RenderPassResource()
        {
            s_device.destroyRenderPass(m_renderPass);
        }

        vk::RenderPass m_renderPass = nullptr;
        uint32_t handle = 0;
    };

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

    struct UniformLayoutResource
    {
        UniformLayoutResource(const UniformLayoutDescription& desc)
        {
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;

            for (auto uniformDesc : desc.m_layout)
            {
                vk::DescriptorSetLayoutBinding layoutBinding = {};
                layoutBinding.setBinding(uniformDesc.binding);
                layoutBinding.setDescriptorCount(uniformDesc.count);
                layoutBinding.setStageFlags(MapShaderStageForVulkan(uniformDesc.stage));
                layoutBinding.setDescriptorType(MapUniformTypeForVulkan(uniformDesc.type));

                descriptorSetLayoutBindings.push_back(layoutBinding);
            }

            vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.setBindingCount(desc.m_layout.size());
            descriptorSetLayoutCreateInfo.setPBindings(descriptorSetLayoutBindings.data());

            auto createDescriptorSetLayoutResult = s_device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
            VK_ASSERT(createDescriptorSetLayoutResult);
            m_descriptorSetLayout = createDescriptorSetLayoutResult.value;
        }

        ~UniformLayoutResource()
        {
            s_device.destroyDescriptorSetLayout(m_descriptorSetLayout);
        }

        uint32_t handle = 0;
        vk::DescriptorSetLayout m_descriptorSetLayout = nullptr;
    };

    struct PipelineResource
    {
        PipelineResource(const GraphicsPipelineDescription& desc)
        {
            std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos = {};
            for (auto shader : desc.shaders)
            {
                ShaderResource* shaderResource = s_shaderHandlePool.FetchResource(shader.id);
                shaderStageCreateInfos.push_back(shaderResource->GetShaderStageCreateInfo());
            }

            auto bindingDesc = CreateBindingDescription(desc.vertexBindings);
            auto attributeDescs = CreateVertexInputAttributeDescriptions(desc.vertexBindings);

            vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
            vertexInputStateCreateInfo.setVertexBindingDescriptionCount(1);
            vertexInputStateCreateInfo.setPVertexBindingDescriptions(&bindingDesc);

            vertexInputStateCreateInfo.setVertexAttributeDescriptionCount(attributeDescs.size());
            vertexInputStateCreateInfo.setPVertexAttributeDescriptions(attributeDescs.data());

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

            vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};

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
                vk::DynamicState::eScissor,
            };

            vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
            dynamicStateCreateInfo.setDynamicStateCount(dynamicStates.size());
            dynamicStateCreateInfo.setPDynamicStates(dynamicStates.data());
            
            /*
            Pipeline Layout
            */
            m_descriptorSetLayouts.resize(desc.uniformBindings.m_layouts.size());
            for (size_t i = 0; i < m_descriptorSetLayouts.size(); i++)
            {
                m_descriptorSetLayouts[i] = s_uniformLayoutHandlePool.FetchResource(desc.uniformBindings.m_layouts[i].id)->m_descriptorSetLayout;
            }

            vk::PipelineLayoutCreateInfo layoutCreateInfo = {};
            layoutCreateInfo.setPSetLayouts(m_descriptorSetLayouts.data());
            layoutCreateInfo.setSetLayoutCount(m_descriptorSetLayouts.size());

            auto createPipelineLayoutResult = s_device.createPipelineLayout(layoutCreateInfo);
            VK_ASSERT(createPipelineLayoutResult);
            m_pipelineLayout = createPipelineLayoutResult.value;

            vk::GraphicsPipelineCreateInfo pipelineCreateInfo = {};
            pipelineCreateInfo.setStageCount(shaderStageCreateInfos.size());
            pipelineCreateInfo.setPStages(shaderStageCreateInfos.data());
            pipelineCreateInfo.setPVertexInputState(&vertexInputStateCreateInfo);
            pipelineCreateInfo.setPInputAssemblyState(&inputAssemblyStateCreateInfo);
            pipelineCreateInfo.setPViewportState(&viewportStateCreateInfo);
            pipelineCreateInfo.setPRasterizationState(&rasterizationStateCreateInfo);
            pipelineCreateInfo.setPMultisampleState(&multisampleStateCreateInfo);
            pipelineCreateInfo.setPDepthStencilState(&depthStencilStateCreateInfo);
            pipelineCreateInfo.setPColorBlendState(&colorBlendStateCreateInfo);
            pipelineCreateInfo.setPDynamicState(&dynamicStateCreateInfo);
            pipelineCreateInfo.setLayout(m_pipelineLayout);
            pipelineCreateInfo.setRenderPass(s_renderPassDefault);
            pipelineCreateInfo.setSubpass(0);

            auto createGraphicsPipelineResult = s_device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
            VK_ASSERT(createGraphicsPipelineResult);
            m_pipeline = createGraphicsPipelineResult.value;
        }

        ~PipelineResource()
        {
            s_device.waitIdle();

            s_device.destroyPipelineLayout(m_pipelineLayout);
            s_device.destroyPipeline(m_pipeline);
        }

        vk::VertexInputBindingDescription CreateBindingDescription(const VertexBindings& bindings)
        {
            vk::VertexInputBindingDescription vertexInputBindingDescription = {};
            vertexInputBindingDescription.setBinding(0);
            vertexInputBindingDescription.setInputRate(MapBindingTypeForVulkan(bindings.m_bindingType));
            vertexInputBindingDescription.setStride(bindings.m_strideSize);

            return vertexInputBindingDescription;
        }

        std::vector<vk::VertexInputAttributeDescription> CreateVertexInputAttributeDescriptions(const VertexBindings& bindings)
        {
            std::vector<vk::VertexInputAttributeDescription> results = {};
            for (size_t i = 0; i < bindings.m_layout.size(); i++)
            {
                auto attributeInfo = bindings.m_layout[i];
                vk::VertexInputAttributeDescription attributeDesc = {};
                attributeDesc.setBinding(bindings.m_bindingPosition);
                attributeDesc.setFormat(MapTypeFormatForVulkan(attributeInfo.type));
                attributeDesc.setLocation(attributeInfo.location);
                attributeDesc.setOffset(attributeInfo.offset);

                results.push_back(attributeDesc);
            }

            return results;
        }

        vk::VertexInputRate MapBindingTypeForVulkan(BindingType bindingType)
        {
            switch (bindingType)
            {
            case BindingType::Vertex:
                return vk::VertexInputRate::eVertex;
            case BindingType::Instance:
                return vk::VertexInputRate::eInstance;
            }
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
        vk::PipelineLayout m_pipelineLayout = nullptr;
        std::vector<vk::DescriptorSetLayout> m_descriptorSetLayouts;
    };

    struct BufferResource
    {
        BufferResource(const BufferDescription& desc)
        {
            m_size = desc.size;
            m_storageMode = desc.storageMode;
            m_usage = desc.usage;

            // If Dynamic, No Staging Buffer, Use Host Visible Buffer
            if (desc.storageMode == BufferStorageMode::Dynamic)
            {
                size_t actualSize = m_size;
                
                // Uniform Buffer With Dynamic Storage Mode
                if (desc.usage == BufferUsage::UniformBuffer)
                {
                    actualSize = s_swapChainImages.size() * m_size;
                }

                CreateVulkanBuffer(
                    actualSize,
                    MapBufferUsageForVulkan(desc.usage),
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
                    m_buffer, 
                    m_deviceMemory);
            }
            else if (desc.storageMode == BufferStorageMode::Static)
            {
                CreateVulkanBuffer(
                    desc.size, 
                    vk::BufferUsageFlagBits::eTransferDst | MapBufferUsageForVulkan(desc.usage), 
                    vk::MemoryPropertyFlagBits::eDeviceLocal, 
                    m_buffer, 
                    m_deviceMemory);
            }
        }

        ~BufferResource()
        {
            s_device.waitIdle();
            s_device.destroyBuffer(m_buffer);
            s_device.freeMemory(m_deviceMemory);
        }

        vk::BufferUsageFlags MapBufferUsageForVulkan(const BufferUsage& usage)
        {
            switch (usage)
            {
            case BufferUsage::VertexBuffer:
                return vk::BufferUsageFlagBits::eVertexBuffer;
            case BufferUsage::UniformBuffer:
                return vk::BufferUsageFlagBits::eUniformBuffer;
            case BufferUsage::IndexBuffer:
                return vk::BufferUsageFlagBits::eIndexBuffer;
            }
        }

        void Update(size_t offset, size_t size, void* data)
        {
            if (m_usage == BufferUsage::UniformBuffer)
            {
                Map(offset + (s_currentImageIndex * m_size), size);
            }
            else
            {
                Map(offset, size);
            }
            memcpy(m_mappedPtr, data, size);
            Unmap();
        }

        void Map(size_t offset, size_t size)
        {
            if (m_storageMode == BufferStorageMode::Dynamic)
            {
                auto mapMemoryResult = s_device.mapMemory(m_deviceMemory, offset, size);
                VK_ASSERT(mapMemoryResult);
                m_mappedPtr = mapMemoryResult.value;
            }
            else
            {
                m_mappedSize = size;
                m_mappedOffset = offset;

                CreateVulkanBuffer(
                    m_size,
                    vk::BufferUsageFlagBits::eTransferSrc,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                    m_stagingBuffer,
                    m_stagingDeviceMemory);

                auto mapMemoryResult = s_device.mapMemory(m_stagingDeviceMemory, offset, size);
                VK_ASSERT(mapMemoryResult);
                m_mappedPtr = mapMemoryResult.value;
            }
        }

        void Unmap()
        {
            if (m_storageMode == BufferStorageMode::Dynamic)
            {
                s_device.unmapMemory(m_deviceMemory);
            }
            else
            {
                // Unmap First
                s_device.unmapMemory(m_stagingDeviceMemory);
                // Copy To device memory
                auto oneTimeCommandBuffer = BeginOneTimeCommandBuffer();

                vk::BufferCopy bufferCopy = {};
                bufferCopy.setSize(m_mappedSize);
                bufferCopy.setDstOffset(m_mappedOffset);
                bufferCopy.setSrcOffset(0);

                oneTimeCommandBuffer.copyBuffer(m_stagingBuffer, m_buffer, bufferCopy);

                EndOneTimeCommandBuffer(oneTimeCommandBuffer);

                // Clear Stage Buffer
                s_device.destroyBuffer(m_stagingBuffer);
                s_device.freeMemory(m_stagingDeviceMemory);
            }
        }

        void* GetMappedPointer() const
        {
            return m_mappedPtr;
        }

        size_t GetSize()
        {
            return m_size;
        }

        uint32_t handle = 0;

        void* m_mappedPtr = nullptr;
        size_t m_mappedSize = 0;
        size_t m_mappedOffset = 0;

        size_t m_size = 0;
        vk::Buffer m_buffer = nullptr;
        vk::DeviceMemory m_deviceMemory = nullptr;
        vk::Buffer m_stagingBuffer = nullptr;
        vk::DeviceMemory m_stagingDeviceMemory = nullptr;
        BufferStorageMode m_storageMode = BufferStorageMode::Dynamic;
        BufferUsage m_usage;
    };

    struct ImageResource
    {
        ImageResource(const ImageDescription& desc)
        {
            m_width = desc.width;
            m_height = desc.height;

            vk::ImageCreateInfo imageCreateInfo = {};
            vk::Extent3D imageExtent = {};
            imageExtent.setWidth(desc.width);
            imageExtent.setHeight(desc.height);
            imageExtent.setDepth(desc.depth);
            imageCreateInfo.setExtent(imageExtent);
            imageCreateInfo.setFormat(MapFormatForVulkan(desc.format));
            imageCreateInfo.setImageType(MapImageTypeForVulkan(desc.type));
            imageCreateInfo.setUsage(MapImageUsageForVulkan(desc.usage));
            imageCreateInfo.setMipLevels(desc.mipLevels);

            if (!desc.readOrWriteByCPU)
            {
                imageCreateInfo.setTiling(vk::ImageTiling::eOptimal);
                imageCreateInfo.setInitialLayout(vk::ImageLayout::eUndefined);
            }
            else
            {
                imageCreateInfo.setTiling(vk::ImageTiling::eLinear);
                imageCreateInfo.setInitialLayout(vk::ImageLayout::ePreinitialized);
            }

            imageCreateInfo.setArrayLayers(1);
            imageCreateInfo.setSamples(MapSampleCountForVulkan(desc.sampleCount));
            // TODO Support Ray Tracing Queue
            imageCreateInfo.setSharingMode(vk::SharingMode::eExclusive);           

            auto createImageResult = s_device.createImage(imageCreateInfo);
            VK_ASSERT(createImageResult);
            m_image = createImageResult.value;

            vk::MemoryRequirements memRequirements = s_device.getImageMemoryRequirements(m_image);
            m_memSize = memRequirements.size;

            vk::MemoryAllocateInfo memAllocInfo = {};
            memAllocInfo.setAllocationSize(memRequirements.size);
            if (!desc.readOrWriteByCPU)
            {
                memAllocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
            }
            else
            {
                memAllocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
            }

            auto allocateMemoryResult = s_device.allocateMemory(memAllocInfo);
            VK_ASSERT(allocateMemoryResult);
            m_deviceMemory = allocateMemoryResult.value;

            s_device.bindImageMemory(m_image, m_deviceMemory, 0);
        }

        ~ImageResource()
        {
            s_device.freeMemory(m_deviceMemory);
            s_device.destroyImage(m_image);
        }

        vk::SampleCountFlagBits MapSampleCountForVulkan(const ImageSampleCount& sampleCount)
        {
            switch (sampleCount)
            {
            case ImageSampleCount::Sample1:
                return vk::SampleCountFlagBits::e1;
            case ImageSampleCount::Sample2:
                return vk::SampleCountFlagBits::e2;
            case ImageSampleCount::Sample4:
                return vk::SampleCountFlagBits::e4;
            default:
                assert(false);
                return vk::SampleCountFlagBits::e1;
            }
        }

        vk::ImageUsageFlags MapImageUsageForVulkan(const ImageUsage& imageUsage)
        {
            switch (imageUsage)
            {
            case ImageUsage::SampledImage:
                return vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            case ImageUsage::AttachmentImage:
                return vk::ImageUsageFlagBits::eColorAttachment;
            default:
                assert(false);
                return vk::ImageUsageFlagBits::eSampled;
            }
        }

        vk::ImageType MapImageTypeForVulkan(const ImageType& imageType)
        {
            switch (imageType)
            {
            case ImageType::Image2D:
                return vk::ImageType::e2D;
            default:
                assert(false);
                return vk::ImageType::e2D;
            }
        }

        uint32_t handle = 0;

        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_depth = 0;

        uint32_t m_memSize = 0;

        vk::DeviceMemory m_deviceMemory = nullptr;
        vk::Image m_image = nullptr;
    };

    struct UniformResource
    {
        UniformResource(const UniformDescription& desc)
        {
            m_layout = desc.m_layout;
            m_storageMode = desc.m_storageMode;

            auto descriptorSetCount = desc.m_storageMode == UniformStorageMode::Dynamic ? s_swapChainImages.size() : 1;

            // Create Descriptor Sets
            vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
            descriptorSetAllocateInfo.setDescriptorPool(s_descriptorPoolDefault);
            descriptorSetAllocateInfo.setDescriptorSetCount(descriptorSetCount);
            UniformLayoutResource* uniformLayoutResource = s_uniformLayoutHandlePool.FetchResource(desc.m_layout.id);

            std::vector<vk::DescriptorSetLayout> layouts(descriptorSetCount, uniformLayoutResource->m_descriptorSetLayout);
            descriptorSetAllocateInfo.setPSetLayouts(layouts.data());

            auto allocateDescriptorSetsResult = s_device.allocateDescriptorSets(descriptorSetAllocateInfo);
            VK_ASSERT(allocateDescriptorSetsResult);
            m_descriptorSets = allocateDescriptorSetsResult.value;

            for (size_t i = 0; i < descriptorSetCount; i++)
            {
                for (size_t j = 0; j < desc.m_atrributes.size(); j++)
                {
                    s_device.waitIdle();

                    auto attribute = desc.m_atrributes[j];
                    BufferResource* bufferResource = s_bufferHandlePool.FetchResource(attribute.buffer.id);

                    if (i == 0)
                    {
                        m_atrributes[attribute.binding] = attribute;
                    }

                    vk::DescriptorBufferInfo bufferInfo = {};
                    bufferInfo.setBuffer(bufferResource->m_buffer);
                    bufferInfo.setOffset(attribute.offset + (bufferResource->m_size * i));
                    bufferInfo.setRange(attribute.range);

                    vk::WriteDescriptorSet writeDescriptorSet = {};
                    writeDescriptorSet.setDescriptorCount(1);
                    writeDescriptorSet.setPBufferInfo(&bufferInfo);
                    writeDescriptorSet.setDstBinding(attribute.binding);
                    writeDescriptorSet.setDstArrayElement(0);
                    writeDescriptorSet.setDstSet(m_descriptorSets[i]);
                    writeDescriptorSet.setDescriptorType(vk::DescriptorType::eUniformBuffer);

                    s_device.updateDescriptorSets(writeDescriptorSet, nullptr);
                }
            }
        }

        ~UniformResource()
        {
            s_device.waitIdle();
            s_device.freeDescriptorSets(s_descriptorPoolDefault, m_descriptorSets.size(), m_descriptorSets.data());
        }

        uint32_t handle = 0;

        UniformLayout m_layout;
        UniformStorageMode m_storageMode;
        std::map<uint32_t, UniformAtrribute> m_atrributes;

        std::vector<vk::DescriptorSet> m_descriptorSets;
    };

    /*
    =================================================Implementation===========================================================
    */

    /*
    Resource Management
    */

    Pipeline CreatePipeline(const GraphicsPipelineDescription& desc)
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

    RenderPass CreateRenderPass(const RenderPassDescription& desc)
    {
        RenderPass result = RenderPass();

        RenderPassResource* renderPassResource = new RenderPassResource(desc);
        result.id = s_renderPassHandlePool.AllocateHandle(renderPassResource);

        renderPassResource->handle = result.id;

        return result;
    }

    Buffer CreateBuffer(const BufferDescription& desc)
    {
        Buffer result = Buffer();

        BufferResource* bufferResource = new BufferResource(desc);
        result.id = s_bufferHandlePool.AllocateHandle(bufferResource);

        bufferResource->handle = result.id;

        return result;
    }

    Image CreateImage(const ImageDescription& desc)
    {
        Image result = Image();

        ImageResource* imageResource = new ImageResource(desc);
        result.id = s_imageHandlePool.AllocateHandle(imageResource);

        imageResource->handle = result.id;

        return result;
    }

    UniformLayout CreateUniformLayout(const UniformLayoutDescription& desc)
    {
        UniformLayout result = UniformLayout();

        UniformLayoutResource* uniformLayoutResource = new UniformLayoutResource(desc);
        result.id = s_uniformLayoutHandlePool.AllocateHandle(uniformLayoutResource);

        uniformLayoutResource->handle = result.id;

        return result;
    }

    Uniform CreateUniform(const UniformDescription& desc)
    {
        Uniform result = Uniform();

        UniformResource* uniformResource = new UniformResource(desc);
        result.id = s_uniformHandlePool.AllocateHandle(uniformResource);

        uniformResource->handle = result.id;

        return result;
    }

    void DestroyShader(const Shader& shader)
    {
        s_shaderHandlePool.FreeHandle(shader.id);
    }

    void DestroyPipeline(const Pipeline& pipeline)
    {
        s_pipelineHandlePool.FreeHandle(pipeline.id);
    }

    void DestroyRenderPass(const RenderPass& renderPass)
    {
        s_renderPassHandlePool.FreeHandle(renderPass.id);
    }

    void DestroyBuffer(const Buffer& buffer)
    {
        s_bufferHandlePool.FreeHandle(buffer.id);
    }

    void DestroyImage(const Image& image)
    {
        s_imageHandlePool.FreeHandle(image.id);
    }

    void DestroyUniformLayout(const UniformLayout& uniformLayout)
    {
        s_uniformLayoutHandlePool.FreeHandle(uniformLayout.id);
    }

    void DestroyUniform(const Uniform& uniform)
    {
        s_uniformHandlePool.FreeHandle(uniform.id);
    }

    /*
    Buffer Operation
    */

    //void* MapBuffer(const Buffer& buffer, size_t offset, size_t size)
    //{
    //    BufferResource* bufferResource = s_bufferHandlePool.FetchResource(buffer.id);
    //    bufferResource->Map(offset, size);
    //    return bufferResource->GetMappedPointer();
    //}

    //void UnmapBuffer(const Buffer& buffer)
    //{
    //    BufferResource* bufferResource = s_bufferHandlePool.FetchResource(buffer.id);
    //    bufferResource->Unmap();
    //}
    size_t GetMinimumUniformBufferAlignment()
    {
        return s_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    }

    size_t AlignmentSize(size_t size, size_t alignment)
    {
        if (size <= alignment)
        {
            return alignment;
        }

        return size + (size % alignment);
    }

    size_t UniformAlign(size_t size)
    {
        return AlignmentSize(size, s_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
    }

    void UpdateBuffer(Buffer buffer, size_t offset, size_t size, void* data)
    {
        BufferResource* bufferResource = s_bufferHandlePool.FetchResource(buffer.id);
        bufferResource->Update(offset, size, data);
    }

    void UpdateImageMemory(Image image, void* data)
    {
        ImageResource* imageResource = s_imageHandlePool.FetchResource(image.id);


        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferDeviceMemory;

        CreateVulkanBuffer(imageResource->m_memSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferDeviceMemory);

        auto mapMemoryResult = s_device.mapMemory(stagingBufferDeviceMemory, 0, imageResource->m_memSize);
        VK_ASSERT(mapMemoryResult);
        memcpy(mapMemoryResult.value, data, imageResource->m_memSize);
        s_device.unmapMemory(stagingBufferDeviceMemory);


    }

    void BindUniform(Uniform uniform, uint32_t set)
    {
        UniformResource* uniformResource = s_uniformHandlePool.FetchResource(uniform.id);
        if (uniformResource->m_storageMode == UniformStorageMode::Dynamic)
        {
            s_currentDescriptors[set] = uniformResource->m_descriptorSets[s_currentImageIndex];
        }
        else
        {
            s_currentDescriptors[set] = uniformResource->m_descriptorSets[0];
        }
    }

    void UpdateUniformBuffer(Uniform uniform, uint32_t binding, void* data)
    {
        UniformResource* uniformResource = s_uniformHandlePool.FetchResource(uniform.id);
        auto attribute = uniformResource->m_atrributes[binding];
        GFX::UpdateBuffer(attribute.buffer, attribute.offset, attribute.range, data);
    }

    /*
    Operations
    */
    void ApplyPipeline(Pipeline pipeline)
    {
        s_currentDescriptors.clear();

        PipelineResource* pipelineResource = s_pipelineHandlePool.FetchResource(pipeline.id);
        s_currentPipleline = pipelineResource;

        s_commandBuffersDefault[s_currentImageIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, pipelineResource->m_pipeline);
    }

    void BindIndexBuffer(Buffer buffer, size_t offset, IndexType indexType)
    {
        BufferResource* bufferResource = s_bufferHandlePool.FetchResource(buffer.id);
        s_commandBuffersDefault[s_currentImageIndex].bindIndexBuffer(bufferResource->m_buffer, offset, MapIndexTypeFormatForVulkan(indexType));
    }

    void BindVertexBuffer(Buffer buffer, size_t offset, uint32_t binding)
    {
        BufferResource* bufferResource = s_bufferHandlePool.FetchResource(buffer.id);
        vk::DeviceSize vkOffset = {offset};
        s_commandBuffersDefault[s_currentImageIndex].bindVertexBuffers(binding, 1, &bufferResource->m_buffer, &vkOffset);
    }

    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        std::vector<vk::DescriptorSet> descriptorSets;
        for (int i = 0; i < s_currentDescriptors.size(); i++)
        {
            descriptorSets.push_back(s_currentDescriptors[i]);
        }
        s_commandBuffersDefault[s_currentImageIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, s_currentPipleline->m_pipelineLayout, 0, descriptorSets, nullptr);

        s_commandBuffersDefault[s_currentImageIndex].draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
    {
        std::vector<vk::DescriptorSet> descriptorSets;
        for (int i = 0; i < s_currentDescriptors.size(); i++)
        {
            descriptorSets.push_back(s_currentDescriptors[i]);
        }
        s_commandBuffersDefault[s_currentImageIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, s_currentPipleline->m_pipelineLayout, 0, descriptorSets, nullptr);

        s_commandBuffersDefault[s_currentImageIndex].drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void SetViewport(float x, float y, float w, float h)
    {
        vk::Viewport newViewport = {};
        newViewport.setX(x);
        newViewport.setY(y);
        newViewport.setWidth(w);
        newViewport.setHeight(h);

        s_commandBuffersDefault[s_currentImageIndex].setViewport(0, 1, &newViewport);
        
    }

    void SetScissor(float x, float y, float w, float h)
    {
        vk::Rect2D scissor = {};
        scissor.setOffset({ static_cast<int32_t>(x), static_cast<int32_t>(y) });
        scissor.setExtent({ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });
        
        s_commandBuffersDefault[s_currentImageIndex].setScissor(0, scissor);
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
        s_physicalDeviceMemoryProperties = s_physicalDevice.getMemoryProperties();
        s_physicalDeviceProperties = s_physicalDevice.getProperties();

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
        CreateDefaultRenderPass();
        CreateSwapChainFramebuffers();
        CreateCommandPoolDefault();
        CreateCommandBuffersDefault();
        CreateSyncObjects();
        CreateDescriptorPoolDefault();
    }

    bool BeginFrame()
    {
        s_device.waitForFences(1, &s_inFlightFences[s_currentFrame], false, UINT64_MAX);
        s_device.resetFences(1, &s_inFlightFences[s_currentFrame]);

        auto acquireNextImageResult = s_device.acquireNextImageKHR(s_swapChain, UINT64_MAX, s_imageAvailableSemaphores[s_currentFrame], nullptr);
        
        if (acquireNextImageResult.result == vk::Result::eErrorOutOfDateKHR || acquireNextImageResult.result == vk::Result::eSuboptimalKHR)
        {
            RecreateSwapChain();
            return false;
        }
        else
        {
            VK_ASSERT(acquireNextImageResult);
        }

        s_currentImageIndex = acquireNextImageResult.value;

        vk::CommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        auto commandBufferBeginResult = s_commandBuffersDefault[s_currentImageIndex].begin(commandBufferBeginInfo);
        assert(commandBufferBeginResult == vk::Result::eSuccess);
        return true;
    }

    void BeginDefaultRenderPass()
    {
        vk::RenderPassBeginInfo renderPassBeginInfo = {};
        
        renderPassBeginInfo.setRenderPass(s_renderPassDefault);
        renderPassBeginInfo.setFramebuffer(s_swapChainFrameBuffers[s_currentImageIndex]);
        
        vk::Rect2D area = {};
        area.setExtent(s_swapChainImageExtent);
        area.setOffset({ 0, 0 });
        renderPassBeginInfo.setRenderArea(area);

        vk::ClearColorValue colorValue = {};
        colorValue.setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });
        vk::ClearValue clearColor = {};
        clearColor.setColor(colorValue);
        renderPassBeginInfo.setClearValueCount(1);
        renderPassBeginInfo.setPClearValues(&clearColor);
        
        s_commandBuffersDefault[s_currentImageIndex].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    }

    void EndRenderPass()
    {
        s_commandBuffersDefault[s_currentImageIndex].endRenderPass();
    }

    void EndFrame()
    {
        s_commandBuffersDefault[s_currentImageIndex].end();

        /*
        Submit Commands
        */
        vk::SubmitInfo submitInfo = {};
        
        static vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

        submitInfo.setWaitSemaphoreCount(1);
        submitInfo.setPWaitSemaphores(&s_imageAvailableSemaphores[s_currentFrame]);
        submitInfo.setPWaitDstStageMask(waitStages);
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&s_commandBuffersDefault[s_currentImageIndex]);
        submitInfo.setSignalSemaphoreCount(1);
        submitInfo.setPSignalSemaphores(&s_renderFinishedSemaphores[s_currentFrame]);

        s_device.resetFences(s_inFlightFences[s_currentFrame]);
        auto submitResult = s_graphicsQueueDefault.submit(submitInfo, s_inFlightFences[s_currentFrame]);
        assert(submitResult == vk::Result::eSuccess);

        vk::PresentInfoKHR presentInfo = {};
        presentInfo.setWaitSemaphoreCount(1);
        presentInfo.setPWaitSemaphores(&s_renderFinishedSemaphores[s_currentFrame]);
        presentInfo.setPSwapchains(&s_swapChain);
        presentInfo.setPImageIndices(&s_currentImageIndex);
        presentInfo.setSwapchainCount(1);

        auto presentResult = s_presentQueueDefault.presentKHR(presentInfo);
        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
        {
            RecreateSwapChain();
        }
        else
        {
            assert(presentResult == vk::Result::eSuccess);
        }

        s_currentFrame = (s_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Shutdown()
    {
        s_device.waitIdle();

        s_device.destroyDescriptorPool(s_descriptorPoolDefault);

        for (auto fence : s_inFlightFences)
        {
            s_device.destroyFence(fence);
        }

        for (auto semaphore : s_imageAvailableSemaphores)
        {
            s_device.destroySemaphore(semaphore);
        }

        for (auto semaphore : s_renderFinishedSemaphores)
        {
            s_device.destroySemaphore(semaphore);
        }

        s_device.destroyCommandPool(s_commandPoolDefault);

        for (auto framebuffer : s_swapChainFrameBuffers)
        {
            vkDestroyFramebuffer(s_device, framebuffer, nullptr);
        }

        s_device.destroyRenderPass(s_renderPassDefault);

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

    void CreateDefaultRenderPass()
    {
        vk::AttachmentDescription colorAttachment = {};
        colorAttachment.setFormat(s_swapChainImageFormat);
        colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
        colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
        colorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

        vk::AttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.setAttachment(0);
        colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        vk::SubpassDescription subpassDescription = {};
        subpassDescription.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
        subpassDescription.setColorAttachmentCount(1);
        subpassDescription.setPColorAttachments(&colorAttachmentRef);

        vk::SubpassDependency dependency = {};
        dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
        dependency.setDstSubpass(0);
        dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

        vk::RenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.setAttachmentCount(1);
        renderPassCreateInfo.setPAttachments(&colorAttachment);
        renderPassCreateInfo.setSubpassCount(1);
        renderPassCreateInfo.setPSubpasses(&subpassDescription);
        renderPassCreateInfo.setDependencyCount(1);
        renderPassCreateInfo.setPDependencies(&dependency);
        
        auto createRenderPassResult = s_device.createRenderPass(renderPassCreateInfo);
        VK_ASSERT(createRenderPassResult);
        s_renderPassDefault = createRenderPassResult.value;
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

    void RecreateSwapChain()
    {
        s_device.waitIdle();

        for (auto framebuffer : s_swapChainFrameBuffers)
        {
            vkDestroyFramebuffer(s_device, framebuffer, nullptr);
        }

        for (auto imageView : s_swapChainImageViews)
        {
            vkDestroyImageView(s_device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(s_device, s_swapChain, nullptr);

        s_swapChainFrameBuffers.clear();
        s_swapChainImageViews.clear();
        s_swapChainImages.clear();

        CreateSwapChain();
        CreateImageViews();
        CreateSwapChainFramebuffers();
    }

    void CreateSwapChainFramebuffers()
    {
        s_swapChainFrameBuffers.resize(s_swapChainImageViews.size());
        for (size_t i = 0; i < s_swapChainImageViews.size(); i++)
        {
            const VkImageView* imageView = &s_swapChainImageViews[i];

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = s_renderPassDefault;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = imageView;
            framebufferInfo.width = s_swapChainImageExtent.width;
            framebufferInfo.height = s_swapChainImageExtent.height;
            framebufferInfo.layers = 1;

            auto result = vkCreateFramebuffer(s_device, &framebufferInfo, nullptr, &s_swapChainFrameBuffers[i]);
            assert(result == VK_SUCCESS);
        }
    }

    void CreateCommandPoolDefault()
    {
        vk::CommandPoolCreateInfo commandPoolCreateInfo = {};
        commandPoolCreateInfo.setQueueFamilyIndex(s_graphicsFamily);
        commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        auto createCommandPoolResult = s_device.createCommandPool(commandPoolCreateInfo);
        VK_ASSERT(createCommandPoolResult);

        s_commandPoolDefault = createCommandPoolResult.value;
    }

    void CreateCommandBuffersDefault()
    {
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.setCommandBufferCount(s_swapChainImages.size());
        commandBufferAllocateInfo.setCommandPool(s_commandPoolDefault);
        commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);

        auto allocateCommandBuffersResult = s_device.allocateCommandBuffers(commandBufferAllocateInfo);
        VK_ASSERT(allocateCommandBuffersResult);
        s_commandBuffersDefault = allocateCommandBuffersResult.value;
    }

    void CreateDescriptorPoolDefault()
    {
        vk::DescriptorPoolSize uniformmBufferPoolSize = {};
        uniformmBufferPoolSize.setType(vk::DescriptorType::eUniformBuffer);
        uniformmBufferPoolSize.setDescriptorCount(500);

        vk::DescriptorPoolSize texturePoolSize = {};
        texturePoolSize.setType(vk::DescriptorType::eSampledImage);
        texturePoolSize.setDescriptorCount(200);

        std::vector<vk::DescriptorPoolSize> sizes;
        sizes.push_back(uniformmBufferPoolSize);
        sizes.push_back(texturePoolSize);

        vk::DescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.setPoolSizeCount(sizes.size());
        poolCreateInfo.setPPoolSizes(sizes.data());
        poolCreateInfo.setMaxSets(1000);
        poolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

        auto createDescriptorPoolResult = s_device.createDescriptorPool(poolCreateInfo);
        VK_ASSERT(createDescriptorPoolResult);
        s_descriptorPoolDefault = createDescriptorPoolResult.value;
    }

    void CreateSyncObjects()
    {        
        s_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        s_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        s_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
        vk::FenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            auto imageAvailableSemaphoreCreateResult = s_device.createSemaphore(semaphoreCreateInfo);
            VK_ASSERT(imageAvailableSemaphoreCreateResult);
            s_imageAvailableSemaphores[i] = imageAvailableSemaphoreCreateResult.value;

            auto renderFinishedSemaphoreCreateResult = s_device.createSemaphore(semaphoreCreateInfo);
            VK_ASSERT(renderFinishedSemaphoreCreateResult);
            s_renderFinishedSemaphores[i] = renderFinishedSemaphoreCreateResult.value;

            auto createInFlightFenceResult = s_device.createFence(fenceCreateInfo);
            VK_ASSERT(createInFlightFenceResult);
            s_inFlightFences[i] = createInFlightFenceResult.value;
        }
    }

    vk::CommandBuffer BeginOneTimeCommandBuffer()
    {
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        commandBufferAllocateInfo.setCommandPool(s_commandPoolDefault);
        commandBufferAllocateInfo.setCommandBufferCount(1);

        auto allocateCommandBufferResult = s_device.allocateCommandBuffers(commandBufferAllocateInfo);
        VK_ASSERT(allocateCommandBufferResult);

        vk::CommandBuffer oneTimeCommandBuffer = allocateCommandBufferResult.value[0];

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        oneTimeCommandBuffer.begin(beginInfo);

        return oneTimeCommandBuffer;
    }

    void EndOneTimeCommandBuffer(vk::CommandBuffer commandBuffer)
    {
        commandBuffer.end();

        vk::SubmitInfo submitInfo = {};
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&commandBuffer);

        s_graphicsQueueDefault.submit(submitInfo, nullptr);
        s_graphicsQueueDefault.waitIdle();

        s_device.freeCommandBuffers(s_commandPoolDefault, commandBuffer);
    }

    vk::Format MapTypeFormatForVulkan(ValueType valueType)
    {
        switch (valueType)
        {
        case ValueType::Float32x2:
            return vk::Format::eR32G32Sfloat;
        case ValueType::Float32x3:
            return vk::Format::eR32G32B32Sfloat;
        case ValueType::UInt16:
            return vk::Format::eR16Uint;
        }
    }

    vk::IndexType MapIndexTypeFormatForVulkan(IndexType indexType)
    {
        switch (indexType)
        {
        case IndexType::UInt16:
            return vk::IndexType::eUint16;
        case IndexType::UInt32:
            return vk::IndexType::eUint32;
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

    vk::DescriptorType MapUniformTypeForVulkan(const UniformType& uniformType)
    {
        switch (uniformType)
        {
        case UniformType::UniformBuffer:
            return vk::DescriptorType::eUniformBuffer;
        case UniformType::Sampler:
            return vk::DescriptorType::eSampler;
        }
    }

    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        for (uint32_t i = 0; i < s_physicalDeviceMemoryProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (s_physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
    }

    void CreateVulkanBuffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
    {
        vk::BufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.setSize(size);
        bufferCreateInfo.setUsage(usage);
        bufferCreateInfo.setSharingMode(vk::SharingMode::eExclusive);

        auto createBufferResult = s_device.createBuffer(bufferCreateInfo);
        VK_ASSERT(createBufferResult);
        buffer = createBufferResult.value;

        vk::MemoryRequirements memRequirements = s_device.getBufferMemoryRequirements(buffer);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.setAllocationSize(memRequirements.size);
        allocInfo.setMemoryTypeIndex(FindMemoryType(memRequirements.memoryTypeBits, properties));

        auto allocResult = s_device.allocateMemory(allocInfo);
        VK_ASSERT(allocResult);
        bufferMemory = allocResult.value;

        s_device.bindBufferMemory(buffer, bufferMemory, 0);
    }

    void TransitionImageLayout(vk::Image img, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
    {
        auto cb = BeginOneTimeCommandBuffer();

        vk::ImageMemoryBarrier barrier = {};
        barrier.setOldLayout(oldLayout);
        barrier.setNewLayout(newLayout);
        barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setImage(img);
        
        vk::ImageSubresourceRange subresourceRange = {};
        subresourceRange.setBaseMipLevel(0);
        subresourceRange.setBaseArrayLayer(0);
        subresourceRange.setLevelCount(1);
        subresourceRange.setLayerCount(1);
        subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
        barrier.setSubresourceRange(subresourceRange);

        barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead); // TODO
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead); // TODO

        // cb.pipelineBarrier()

        EndOneTimeCommandBuffer(cb);
    }

    void CopyBufferToImage(vk::Buffer buffer, vk::Image img, uint32_t width, uint32_t height)
    {
        auto cb = BeginOneTimeCommandBuffer();
        EndOneTimeCommandBuffer(cb);
    }

    uint32_t HashTwoInt(uint32_t a, uint32_t b)
    {
        return (a + b) * (a + b + 1) / 2 + b;
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
            if (format.format == vk::Format::eR8G8B8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
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