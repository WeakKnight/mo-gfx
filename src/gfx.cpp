#include "gfx.h"
#define VULKAN_HPP_ASSERT
#define VULKAN_HPP_NO_EXCEPTIONS

#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <shaderc/shaderc.hpp>

#include <map>
#include <sstream>
#include <fstream>
#include <assert.h>
#include <stdio.h>

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
    struct SamplerResource;
    struct AttachmentResource;

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
    static HandlePool<SamplerResource> s_samplerHandlePool = HandlePool<SamplerResource>(256);

    /*
    Device Instance
    */
    static vk::Instance s_instance = nullptr;
    static vk::PhysicalDevice s_physicalDevice = nullptr;
    static vk::PhysicalDeviceMemoryProperties s_physicalDeviceMemoryProperties;
    static vk::PhysicalDeviceProperties s_physicalDeviceProperties;
    static vk::Device s_device = nullptr;

    static ktxVulkanDeviceInfo s_ktx_device_info;

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

    // TODO RenderPass Abstraction
    // vk::Image s_depthImage;
    // vk::DeviceMemory s_depthImageMemory;
    // vk::ImageView s_depthImageView;

    uint32_t s_currentImageIndex = 0;
    uint32_t s_currentFrame = 0;

    /*
    Current Pipeline
    */
    PipelineResource* s_currentPipleline = nullptr;

    /*
    Swap Chain Frame Buffers
    */
    // std::vector<VkFramebuffer> s_swapChainFrameBuffers;

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

    vk::PhysicalDeviceRayTracingPropertiesKHR s_rayTracingProperties;

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
    // void CreateDepthImage();
    // void CreateDefaultRenderPass();
    // void CreateSwapChainFramebuffers();
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
    vk::Filter MapFilterForVulkan(const FilterMode& filterMode);
    vk::SamplerMipmapMode MapMipmapFilterForVulkan(const FilterMode& filterMode);
    vk::SamplerAddressMode MapWrapModeForVulkan(const WrapMode& wrapMode);
    vk::BorderColor MapBorderColorForVulkan(const BorderColor& borderColor);
    vk::SampleCountFlagBits MapSampleCountForVulkan(const ImageSampleCount& sampleCount);
    vk::Format MapFormatForVulkan(const Format& format);

    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tilling, vk::FormatFeatureFlags features);
    vk::Format FindDepthFormat();
    bool HasStencilComponent(vk::Format format);

    void CreateVulkanBuffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
    void TransitionImageLayout(vk::Image img, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t layerCount);
    void CopyBufferToImage(vk::Buffer buffer, vk::Image img, uint32_t width, uint32_t height, uint32_t layerCount);
    void CreateVulkanImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory);
    vk::ImageView CreateVulkanImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect, vk::ImageViewType type, uint32_t layerCount, uint32_t levelCount);

    uint32_t HashTwoInt(uint32_t a, uint32_t b);

    /*
    ===========================================Internal Struct Definition===================================================
    */
    struct AttachmentResource
    {
        bool isSwapChain = false;
        vk::Format m_format;
        vk::ImageUsageFlags m_usage;
        vk::DeviceMemory m_memory;
        vk::ImageView m_imageView;
        vk::Image m_image;
    };

    struct RenderPassResource
    {
        RenderPassResource(const RenderPassDescription& desc)
        {
            m_width = desc.width;
            m_height = desc.height;
            m_attachments = desc.attachments;

            std::vector<vk::AttachmentDescription> attachmentDescs(desc.attachments.size());
            for (int i = 0; i < desc.attachments.size(); i++)
            {
                auto& attachmentDesc = desc.attachments[i];
                attachmentDescs[i].setFormat(MapFormatForVulkan(attachmentDesc.format));
                attachmentDescs[i].setSamples(MapSampleCountForVulkan(attachmentDesc.samples));
                attachmentDescs[i].setLoadOp(MapLoadOpForVulkan(attachmentDesc.loadAction));
                attachmentDescs[i].setStoreOp(MapStoreOpForVulkan(attachmentDesc.storeAction));
                attachmentDescs[i].setStencilLoadOp(MapLoadOpForVulkan(attachmentDesc.stencilLoadAction));
                attachmentDescs[i].setStencilStoreOp(MapStoreOpForVulkan(attachmentDesc.stencilStoreAction));
                attachmentDescs[i].setInitialLayout(vk::ImageLayout::eUndefined);
                
                if (attachmentDesc.type == AttachmentType::Present)
                {
                    attachmentDescs[i].setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
                    m_attachmentDic[i] = GetSwapChainAttachment(s_currentImageIndex);
                }
                else if (attachmentDesc.type == AttachmentType::DepthStencil)
                {
                    attachmentDescs[i].setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
                    m_attachmentDic[i] = CreateAttachment(attachmentDesc.width, attachmentDesc.height, MapFormatForVulkan(attachmentDesc.format), vk::ImageUsageFlagBits::eDepthStencilAttachment);
                }
                else if (attachmentDesc.type == AttachmentType::Color)
                {
                    attachmentDescs[i].setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
                    m_attachmentDic[i] = CreateAttachment(attachmentDesc.width, attachmentDesc.height, MapFormatForVulkan(attachmentDesc.format), vk::ImageUsageFlagBits::eColorAttachment);
                }
                else
                {
                    assert(false);
                }

                m_clearValues.push_back(MapClearValueForVulkan(attachmentDesc.clearValue));
            }

            std::vector<vk::SubpassDescription> subpassDescs(desc.subpasses.size());

            std::vector<std::vector<vk::AttachmentReference>> colorRefsVector(desc.subpasses.size());
            std::vector <vk::AttachmentReference> depthRefVector(desc.subpasses.size());
            std::vector <std::vector<vk::AttachmentReference>> inputRefsVector(desc.subpasses.size());

            for(int i = 0; i < desc.subpasses.size(); i++)
            {
                auto& subpassDesc = desc.subpasses[i];

                for (auto attachmentIndex : subpassDesc.colorAttachments)
                {
                    vk::AttachmentReference colorRef = { attachmentIndex, vk::ImageLayout::eColorAttachmentOptimal };
                    colorRefsVector[i].push_back(colorRef);
                }

                if (subpassDesc.hasDepth)
                {
                    depthRefVector[i] = { subpassDesc.depthStencilAttachment, vk::ImageLayout::eDepthStencilAttachmentOptimal };
                }

                for (auto inputIndex : subpassDesc.inputAttachments)
                {
                    vk::AttachmentReference inputRef = { inputIndex, vk::ImageLayout::eShaderReadOnlyOptimal };
                    inputRefsVector[i].push_back(inputRef);
                }

                subpassDescs[i].setPipelineBindPoint(MapPipelineBindPointForVulkan(subpassDesc.pipelineType));
                subpassDescs[i].setColorAttachmentCount(colorRefsVector[i].size());
                subpassDescs[i].setPColorAttachments(colorRefsVector[i].data());
                
                if (subpassDesc.hasDepth)
                {
                    subpassDescs[i].setPDepthStencilAttachment(&depthRefVector[i]);
                }

                if (subpassDesc.inputAttachments.size() > 0)
                {
                    subpassDescs[i].setInputAttachmentCount(inputRefsVector[i].size());
                    subpassDescs[i].setPInputAttachments(inputRefsVector[i].data());
                }
            }

            // Build Dependency
            std::vector<vk::SubpassDependency> dependencies(desc.dependencies.size());
            for (int i = 0; i < desc.dependencies.size(); i++)
            {
                auto& dependencyDesc = desc.dependencies[i];

                dependencies[i].setSrcSubpass(dependencyDesc.srcSubpass);
                dependencies[i].setDstSubpass(dependencyDesc.dstSubpass);
                dependencies[i].setSrcStageMask(MapPipelineStageForVulkan(dependencyDesc.srcStage));
                dependencies[i].setDstStageMask(MapPipelineStageForVulkan(dependencyDesc.dstStage));
                dependencies[i].setSrcAccessMask(MapAcessForVulkan(dependencyDesc.srcAccess));
                dependencies[i].setDstAccessMask(MapAcessForVulkan(dependencyDesc.dstAccess));

                dependencies[i].setDependencyFlags(vk::DependencyFlagBits::eByRegion);
            }

            vk::RenderPassCreateInfo renderPassCreateInfo = {};
            renderPassCreateInfo.setAttachmentCount(attachmentDescs.size());
            renderPassCreateInfo.setPAttachments(attachmentDescs.data());
            renderPassCreateInfo.setSubpassCount(subpassDescs.size());
            renderPassCreateInfo.setPSubpasses(subpassDescs.data());
            renderPassCreateInfo.setDependencyCount(dependencies.size());
            renderPassCreateInfo.setPDependencies(dependencies.data());

            auto createRenderPassResult = s_device.createRenderPass(renderPassCreateInfo);
            VK_ASSERT(createRenderPassResult);
            m_renderPass = createRenderPassResult.value;

            CreateFramebuffers();
        }

        ~RenderPassResource()
        {
            for (auto pair : m_attachmentDic)
            {
                DestroyAttachment(pair.second);
            }

            DestroyFramebuffers();

            s_device.destroyRenderPass(m_renderPass);
        }

        void Resize(int width, int height)
        {
            m_width = width;
            m_height = height;

            for (int i = 0; i < m_attachments.size(); i++)
            {
                auto& attachment = m_attachments[i];
                attachment.width = width;
                attachment.height = height;
                
                DestroyAttachment(m_attachmentDic[i]);
                m_attachmentDic[i] = ResizeAttachment(m_attachmentDic[i], width, height);
            }

            DestroyFramebuffers();
            CreateFramebuffers();
        }

        void CreateFramebuffers()
        {
            m_framebuffers.resize(s_swapChainImages.size());

            for (int i = 0; i < s_swapChainImages.size(); i++)
            {
                std::vector<vk::ImageView> imageViews;
                for (int j = 0; j < m_attachments.size(); j++)
                {
                    if (m_attachmentDic[j].isSwapChain)
                    {
                        imageViews.push_back(s_swapChainImageViews[i]);
                    }
                    else
                    {
                        imageViews.push_back(m_attachmentDic[j].m_imageView);
                    }
                }

                vk::FramebufferCreateInfo frameBufferCreateInfo = {};
                frameBufferCreateInfo.setRenderPass(m_renderPass);
                frameBufferCreateInfo.setAttachmentCount(imageViews.size());
                frameBufferCreateInfo.setPAttachments(imageViews.data());
                frameBufferCreateInfo.setWidth(m_width);
                frameBufferCreateInfo.setHeight(m_height);
                frameBufferCreateInfo.setLayers(1);

                auto createFramebufferResult = s_device.createFramebuffer(frameBufferCreateInfo);
                VK_ASSERT(createFramebufferResult);
                m_framebuffers[i] = createFramebufferResult.value;
            }          
        }

        void DestroyFramebuffers()
        {
            for (int i = 0; i < s_swapChainImages.size(); i++)
            {
                s_device.destroyFramebuffer(m_framebuffers[i]);
            }
        }

        AttachmentResource GetSwapChainAttachment(uint32_t imageIndex)
        {
            AttachmentResource result = {};
            result.isSwapChain = true;
            result.m_image = s_swapChainImages[imageIndex];
            result.m_imageView = s_swapChainImageViews[imageIndex];
            
            return result;
        }

        AttachmentResource CreateAttachment(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage)
        {
            AttachmentResource result = {};

            result.m_format = format;
            result.m_usage = usage;

            CreateVulkanImage(width, height, format, vk::ImageTiling::eOptimal, usage | vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, result.m_image, result.m_memory);

            if (usage & vk::ImageUsageFlagBits::eColorAttachment)
            {
                result.m_imageView = CreateVulkanImageView(result.m_image, format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::e2D, 1, 1);
            }
            else if (usage & vk::ImageUsageFlagBits::eDepthStencilAttachment)
            {
                if (HasStencilComponent(format))
                {
                    result.m_imageView = CreateVulkanImageView(result.m_image, format, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, vk::ImageViewType::e2D, 1, 1);
                }
                else
                {
                    result.m_imageView = CreateVulkanImageView(result.m_image, format, vk::ImageAspectFlagBits::eDepth, vk::ImageViewType::e2D, 1, 1);
                }
            }

            return result;
        }

        AttachmentResource ResizeAttachment(AttachmentResource oldAttachment, int width, int height)
        {
            AttachmentResource result = {};
            result.m_format = oldAttachment.m_format;
            result.m_usage = oldAttachment.m_usage;
            result.isSwapChain = oldAttachment.isSwapChain;

            if (!result.isSwapChain)
            {
                CreateVulkanImage(width, height, oldAttachment.m_format, vk::ImageTiling::eOptimal, oldAttachment.m_usage | vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, result.m_image, result.m_memory);
                if (oldAttachment.m_usage & vk::ImageUsageFlagBits::eColorAttachment)
                {
                    result.m_imageView = CreateVulkanImageView(result.m_image, oldAttachment.m_format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::e2D, 1, 1);
                }
                else if (oldAttachment.m_usage & vk::ImageUsageFlagBits::eDepthStencilAttachment)
                {
                    if (HasStencilComponent(oldAttachment.m_format))
                    {
                        result.m_imageView = CreateVulkanImageView(result.m_image, oldAttachment.m_format, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, vk::ImageViewType::e2D, 1, 1);
                    }
                    else
                    {
                        result.m_imageView = CreateVulkanImageView(result.m_image, oldAttachment.m_format, vk::ImageAspectFlagBits::eDepth, vk::ImageViewType::e2D, 1, 1);
                    }
                }
            }

            return result;
        }

        void DestroyAttachment(AttachmentResource& attachment)
        {
            if (attachment.isSwapChain)
            {
                return;
            }

            s_device.waitIdle();

            s_device.destroyImageView(attachment.m_imageView);
            s_device.destroyImage(attachment.m_image);
            s_device.freeMemory(attachment.m_memory);
        }

        vk::AccessFlags MapAcessForVulkan(const Access& access)
        {
            switch (access)
            {
            case Access::ColorAttachmentWrite:
                return vk::AccessFlagBits::eColorAttachmentWrite;
            case Access::ShaderRead:
                return vk::AccessFlagBits::eShaderRead;
            case Access::InputAttachmentRead:
                return vk::AccessFlagBits::eInputAttachmentRead;
            default:
                assert(false);
                return vk::AccessFlagBits::eShaderRead;
            }
        }

        vk::PipelineStageFlags MapPipelineStageForVulkan(const PipelineStage& pipelineStage)
        {
            switch (pipelineStage)
            {
            case PipelineStage::ColorAttachmentOutput:
                return vk::PipelineStageFlagBits::eColorAttachmentOutput;
            case PipelineStage::FragmentShader:
                return vk::PipelineStageFlagBits::eFragmentShader;
            case PipelineStage::VertexShader:
                return vk::PipelineStageFlagBits::eVertexShader;
            case PipelineStage::All:
                return vk::PipelineStageFlagBits::eAllCommands;
            default:
                assert(false);
                return vk::PipelineStageFlagBits::eAllCommands;
            }
        }

        vk::PipelineBindPoint MapPipelineBindPointForVulkan(const PipelineType& pipelineType)
        {
            switch (pipelineType)
            {
            case PipelineType::Graphics:
                return vk::PipelineBindPoint::eGraphics;
            case PipelineType::RayTracing:
                return vk::PipelineBindPoint::eRayTracingNV;
            case PipelineType::Compute:
                return vk::PipelineBindPoint::eCompute;
            default:
                assert(false);
                return vk::PipelineBindPoint::eGraphics;
            }
        }

        vk::AttachmentLoadOp MapLoadOpForVulkan(const AttachmentLoadAction& loadAction)
        {
            switch (loadAction)
            {
            case AttachmentLoadAction::Clear:
                return vk::AttachmentLoadOp::eClear;
            case AttachmentLoadAction::Load:
                return vk::AttachmentLoadOp::eLoad;
            case AttachmentLoadAction::DontCare:
                return vk::AttachmentLoadOp::eDontCare;
            }
        }

        vk::AttachmentStoreOp MapStoreOpForVulkan(const AttachmentStoreAction& storeAction)
        {
            switch (storeAction)
            {
            case AttachmentStoreAction::Store:
                return vk::AttachmentStoreOp::eStore;
            case AttachmentStoreAction::DontCare:
                return vk::AttachmentStoreOp::eDontCare;
            }
        }

        vk::ClearValue MapClearValueForVulkan(const ClearValue& clearValue)
        {
            vk::ClearValue result;

            if (clearValue.hasColor)
            {
                return MapClearColorValueForVulkan(clearValue);
            }
            else
            {
                assert(clearValue.hasDepthStencil);
                return MapClearDepthStencilValueForVulkan(clearValue);
            }
        }

        vk::ClearColorValue MapClearColorValueForVulkan(const ClearValue& clearValue)
        {
            vk::ClearColorValue clearColor = {};
            clearColor.setFloat32({ clearValue.color.r, clearValue.color.g, clearValue.color.b, clearValue.color.a });

            return clearColor;
        }

        vk::ClearDepthStencilValue MapClearDepthStencilValueForVulkan(const ClearValue& clearValue)
        {
            vk::ClearDepthStencilValue clearDepthStencil = {};
            clearDepthStencil.setDepth(clearValue.depth);
            clearDepthStencil.setStencil(clearValue.stencil);

            return clearDepthStencil;
        }

        vk::RenderPass m_renderPass = nullptr;
        std::map<uint32_t, AttachmentResource> m_attachmentDic;
        std::vector<vk::Framebuffer> m_framebuffers;

        std::vector<vk::ClearValue> m_clearValues;

        std::vector<AttachmentDescription> m_attachments;

        uint32_t m_width;
        uint32_t m_height;
       
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
            case ShaderStage::RayGen:
                return shaderc_shader_kind::shaderc_raygen_shader;
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

        class ShaderIncludeCallback : public shaderc::CompileOptions::IncluderInterface
        {
        public:
            // Handles shaderc_include_resolver_fn callbacks.
            shaderc_include_result* GetInclude(const char* requested_source,
                shaderc_include_type type,
                const char* requesting_source,
                size_t include_depth)
            {
                std::string content = ReadFile(requested_source);
                shaderc_include_result result;
                result.content = content.c_str();
                result.content_length = strlen(content.c_str());
                result.source_name = requesting_source;
                result.source_name_length = strlen(requesting_source);
                responses_.push_back(result);
                return &responses_.back();
            }

            // Handles shaderc_include_result_release_fn callbacks.
            void ReleaseInclude(shaderc_include_result* data)
            {
                int b = 2;
            }

            static inline std::string ReadFile(const std::string& filepath) {
                std::ifstream ifs(filepath.c_str());
                std::string content((std::istreambuf_iterator<char>(ifs)),
                    (std::istreambuf_iterator<char>()));
                ifs.close();
                return content;
            }

            std::vector<shaderc_include_result> responses_;
        };

        std::vector<uint32_t> CompileFile(const std::string& sourceName,
            shaderc_shader_kind kind,
            const std::string& source,
            bool optimize)
        {
            shaderc::Compiler compiler;
            shaderc::CompileOptions options;

            // options.SetIncluder(std::unique_ptr<ShaderIncludeCallback>(new ShaderIncludeCallback));

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
                printf("Shader Compiling Failed. Error: %s\n", errorMsg.c_str());
                // spdlog::error("Shader Compiling Failed. Error: {}", errorMsg);
                if (status == shaderc_compilation_status_invalid_stage)
                {
                    printf("Wrong Shader Stage\n");
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
            RenderPassResource* renderPassResource = s_renderPassHandlePool.FetchResource(desc.renderPass.id);

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
            rasterizationStateCreateInfo.setCullMode(MapCullModeForVulkan(desc.cullFace));
            rasterizationStateCreateInfo.setFrontFace(MapFrontFaceForVulkan(desc.fronFace));
            rasterizationStateCreateInfo.setDepthBiasEnable(false);

            vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
            depthStencilStateCreateInfo.setDepthTestEnable(desc.enableDepthTest);
            depthStencilStateCreateInfo.setDepthWriteEnable(true);
            depthStencilStateCreateInfo.setDepthCompareOp(vk::CompareOp::eLess);
            depthStencilStateCreateInfo.setDepthBoundsTestEnable(false);
            depthStencilStateCreateInfo.setStencilTestEnable(desc.enableStencilTest);

            vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
            multisampleStateCreateInfo.setSampleShadingEnable(false);
            multisampleStateCreateInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);

            std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates = {};

            // TODO Blend States
            for (auto blendState: desc.blendStates)
            {
                vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = {};
                colorBlendAttachmentState.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
                colorBlendAttachmentState.setBlendEnable(false);

                colorBlendAttachmentStates.push_back(colorBlendAttachmentState);
            }

            if (desc.blendStates.size() == 0)
            {
                vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = {};
                colorBlendAttachmentState.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
                colorBlendAttachmentState.setBlendEnable(false);

                colorBlendAttachmentStates.push_back(colorBlendAttachmentState);
            }

            vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
            colorBlendStateCreateInfo.setLogicOpEnable(false);
            colorBlendStateCreateInfo.setAttachmentCount(colorBlendAttachmentStates.size());
            colorBlendStateCreateInfo.setPAttachments(colorBlendAttachmentStates.data());

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

            pipelineCreateInfo.setRenderPass(renderPassResource->m_renderPass);
            pipelineCreateInfo.setSubpass(desc.subpass);

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

        vk::FrontFace MapFrontFaceForVulkan(const FrontFace& frontFace)
        {
            switch (frontFace)
            {
            case FrontFace::Clockwise:
                return vk::FrontFace::eClockwise;
            case FrontFace::CounterClockwise:
                return vk::FrontFace::eCounterClockwise;
            }
        }

        vk::CullModeFlags MapCullModeForVulkan(const CullFace& cullFace)
        {
            switch (cullFace)
            {
            case CullFace::Back:
                return vk::CullModeFlagBits::eBack;
            case CullFace::Front:
                return vk::CullModeFlagBits::eFront;
            case CullFace::FrontAndBack:
                return vk::CullModeFlagBits::eFrontAndBack;
            case CullFace::None:
                return vk::CullModeFlagBits::eNone;
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
            case BufferUsage::TransferBuffer:
                return vk::BufferUsageFlagBits::eTransferSrc;
            }
        }

        void Update(size_t offset, size_t size, void* data)
        {
            if (m_usage == BufferUsage::UniformBuffer)
            {
                if (m_storageMode == BufferStorageMode::Dynamic)
                {
                    Map(offset + (s_currentImageIndex * m_size), size);
                }
                else
                {
                    Map(offset, size);
                }
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

    struct SamplerResource
    {
        SamplerResource(const SamplerDescription& desc)
        {
            vk::SamplerCreateInfo samplerCreateInfo = {};

            samplerCreateInfo.setAddressModeU(MapWrapModeForVulkan(desc.wrapU));
            samplerCreateInfo.setAddressModeV(MapWrapModeForVulkan(desc.wrapV));
            samplerCreateInfo.setAddressModeW(MapWrapModeForVulkan(desc.wrapW));

            samplerCreateInfo.setMagFilter(MapFilterForVulkan(desc.magFilter));
            samplerCreateInfo.setMinFilter(MapFilterForVulkan(desc.minFilter));

            if (desc.wrapU == WrapMode::ClampToBorder
             || desc.wrapV == WrapMode::ClampToBorder
             || desc.wrapW == WrapMode::ClampToBorder)
            {
                samplerCreateInfo.setBorderColor(MapBorderColorForVulkan(desc.borderColor));
            }

            if (desc.anisotropyEnable)
            {
                samplerCreateInfo.setAnisotropyEnable(true);
                samplerCreateInfo.setMaxAnisotropy(desc.maxAnisotropy);
            }

            samplerCreateInfo.setUnnormalizedCoordinates(!desc.normalizedCoordinates);

            samplerCreateInfo.setCompareEnable(false);

            samplerCreateInfo.setMipmapMode(MapMipmapFilterForVulkan(desc.mipmapFilter));
            samplerCreateInfo.setMipLodBias(0.0f);
            samplerCreateInfo.setMinLod(0.0f);
            samplerCreateInfo.setMaxLod(desc.maxLod);

            auto createSamplerResult = s_device.createSampler(samplerCreateInfo);
            VK_ASSERT(createSamplerResult);

            m_sampler = createSamplerResult.value;
        }

        ~SamplerResource()
        {
            s_device.waitIdle();
            s_device.destroySampler(m_sampler);
        }
        
        uint32_t handle = 0;
        vk::Sampler m_sampler;
    };

    struct ImageResource
    {
        ImageResource(const ImageDescription& desc)
        {
            m_width = desc.width;
            m_height = desc.height;
            m_format = MapFormatForVulkan(desc.format);

            m_type = desc.type;

            if (desc.type == ImageType::Cube)
            {
                m_layerCount = 6;
            }
            else
            {
                m_layerCount = 1;
            }

            vk::ImageCreateInfo imageCreateInfo = {};
            vk::Extent3D imageExtent = {};
            imageExtent.setWidth(desc.width);
            imageExtent.setHeight(desc.height);
            imageExtent.setDepth(desc.depth);
            imageCreateInfo.setExtent(imageExtent);
            imageCreateInfo.setFormat(m_format);
            imageCreateInfo.setImageType(MapImageTypeForVulkan(desc.type));
            imageCreateInfo.setUsage(MapImageUsageForVulkan(desc.usage));
            imageCreateInfo.setMipLevels(desc.mipLevels);

            if (desc.type == ImageType::Cube)
            {
                imageCreateInfo.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
            }

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

            imageCreateInfo.setArrayLayers(m_layerCount);

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

            m_imageView = CreateVulkanImageView(m_image, m_format, vk::ImageAspectFlagBits::eColor, MapImageViewTypeForVulkan(m_type), m_layerCount, 1);
        }

        ImageResource(const char* path)
        {
            assert(!ktxInitialized);

            ktxResult result;
            ktxTexture* ktxTexture;

            result = ktxTexture_CreateFromNamedFile(path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
            assert(result == KTX_SUCCESS);

            result = ktxTexture_VkUploadEx(ktxTexture, &s_ktx_device_info, &ktxVulkanTexture, VkImageTiling::VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
           
            assert(result == KTX_SUCCESS);

            m_deviceMemory = ktxVulkanTexture.deviceMemory;
            m_image = ktxVulkanTexture.image;

            vk::ImageViewCreateInfo viewInfo;
            // Set the non-default values.
            viewInfo.image = ktxVulkanTexture.image;
            viewInfo.format = static_cast<vk::Format>(ktxVulkanTexture.imageFormat);
            viewInfo.viewType = static_cast<vk::ImageViewType>(ktxVulkanTexture.viewType);
            viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            viewInfo.subresourceRange.layerCount = ktxVulkanTexture.layerCount;
            viewInfo.subresourceRange.levelCount = ktxVulkanTexture.levelCount;

            auto createImageViewResult = s_device.createImageView(viewInfo);
            VK_ASSERT(createImageViewResult);

            m_imageView = createImageViewResult.value;
        }

        ~ImageResource()
        {
            s_device.waitIdle();
            s_device.destroyImageView(m_imageView);
            s_device.freeMemory(m_deviceMemory);
            s_device.destroyImage(m_image);
        }

        vk::ImageUsageFlags MapImageUsageForVulkan(const ImageUsage& imageUsage)
        {
            switch (imageUsage)
            {
            case ImageUsage::SampledImage:
                return vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            case ImageUsage::ColorAttachment:
                return vk::ImageUsageFlagBits::eColorAttachment;
            case ImageUsage::DepthStencilAttachment:
                return vk::ImageUsageFlagBits::eDepthStencilAttachment;
            default:
                assert(false);
                return vk::ImageUsageFlagBits::eSampled;
            }
        }

        vk::ImageViewType MapImageViewTypeForVulkan(const ImageType& imageType)
        {
            switch (imageType)
            {
            case ImageType::Image2D:
                return vk::ImageViewType::e2D;
            case ImageType::Cube:
                return vk::ImageViewType::eCube;
            }
        }

        vk::ImageType MapImageTypeForVulkan(const ImageType& imageType)
        {
            switch (imageType)
            {
            case ImageType::Image2D:
                return vk::ImageType::e2D;
            case ImageType::Cube:
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

        uint32_t m_layerCount = 1;

        ImageType m_type;

        uint32_t m_memSize = 0;

        vk::Format m_format = vk::Format::eR8G8B8A8Snorm;

        vk::DeviceMemory m_deviceMemory = nullptr;
        vk::Image m_image = nullptr;
        vk::ImageView m_imageView = nullptr;

        ktxVulkanTexture ktxVulkanTexture;
        bool ktxInitialized = false;
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
                for (size_t j = 0; j < desc.m_bufferAtrributes.size(); j++)
                {
                    s_device.waitIdle();

                    auto attribute = desc.m_bufferAtrributes[j];
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

                for (size_t j = 0; j < desc.m_inputAttachmentAttributes.size(); j++)
                {
                    auto attribute = desc.m_inputAttachmentAttributes[j];
                    RenderPassResource* renderPassResource =  s_renderPassHandlePool.FetchResource(attribute.renderPass.id);

                    vk::ImageView imageView = {};
                    if (renderPassResource->m_attachmentDic[attribute.attachmentIndex].isSwapChain)
                    {
                        imageView = renderPassResource->GetSwapChainAttachment(i).m_imageView;
                    }
                    else
                    {
                        imageView = renderPassResource->m_attachmentDic[attribute.attachmentIndex].m_imageView;
                    }

                    // SamplerResource* samplerResource = s_samplerHandlePool.FetchResource(attribute.sampler.id);

                    vk::DescriptorImageInfo imageInfo = {};
                    imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
                    // imageInfo.setSampler(samplerResource->m_sampler);
                    imageInfo.setImageView(imageView);

                    vk::WriteDescriptorSet writeDescriptorSet = {};
                    writeDescriptorSet.setDescriptorCount(1);
                    writeDescriptorSet.setPImageInfo(&imageInfo);
                    writeDescriptorSet.setDstBinding(attribute.binding);
                    writeDescriptorSet.setDstArrayElement(0);
                    writeDescriptorSet.setDstSet(m_descriptorSets[i]);
                    writeDescriptorSet.setDescriptorType(vk::DescriptorType::eInputAttachment);

                    s_device.updateDescriptorSets(writeDescriptorSet, nullptr);
                }

                for (size_t j = 0; j < desc.m_sampledAttachmentAttributes.size(); j++)
                {
                    auto attribute = desc.m_sampledAttachmentAttributes[j];
                    RenderPassResource* renderPassResource = s_renderPassHandlePool.FetchResource(attribute.renderPass.id);

                    vk::ImageView imageView = {};
                    if (renderPassResource->m_attachmentDic[attribute.attachmentIndex].isSwapChain)
                    {
                        imageView = renderPassResource->GetSwapChainAttachment(i).m_imageView;
                    }
                    else
                    {
                        imageView = renderPassResource->m_attachmentDic[attribute.attachmentIndex].m_imageView;
                    }

                    SamplerResource* samplerResource = s_samplerHandlePool.FetchResource(attribute.sampler.id);

                    vk::DescriptorImageInfo imageInfo = {};
                    imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
                    imageInfo.setSampler(samplerResource->m_sampler);
                    imageInfo.setImageView(imageView);

                    vk::WriteDescriptorSet writeDescriptorSet = {};
                    writeDescriptorSet.setDescriptorCount(1);
                    writeDescriptorSet.setPImageInfo(&imageInfo);
                    writeDescriptorSet.setDstBinding(attribute.binding);
                    writeDescriptorSet.setDstArrayElement(0);
                    writeDescriptorSet.setDstSet(m_descriptorSets[i]);
                    writeDescriptorSet.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);

                    s_device.updateDescriptorSets(writeDescriptorSet, nullptr);
                }

                for (size_t j = 0; j < desc.m_imageAttributes.size(); j++)
                {
                    s_device.waitIdle();
                    auto attribute = desc.m_imageAttributes[j];
                    ImageResource* imageResource = s_imageHandlePool.FetchResource(attribute.image.id);
                    SamplerResource* samplerResource = s_samplerHandlePool.FetchResource(attribute.sampler.id);

                    vk::DescriptorImageInfo imageInfo = {};
                    imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
                    imageInfo.setSampler(samplerResource->m_sampler);
                    imageInfo.setImageView(imageResource->m_imageView);

                    vk::WriteDescriptorSet writeDescriptorSet = {};
                    writeDescriptorSet.setDescriptorCount(1);
                    writeDescriptorSet.setPImageInfo(&imageInfo);
                    writeDescriptorSet.setDstBinding(attribute.binding);
                    writeDescriptorSet.setDstArrayElement(0);
                    writeDescriptorSet.setDstSet(m_descriptorSets[i]);
                    writeDescriptorSet.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);

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
        std::map<uint32_t, UniformBufferAtrribute> m_atrributes;

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

    Image CreateImageFromKtxTexture(const char* path)
    {
        Image result = Image();
        
        ImageResource* imageResource = new ImageResource(path);
        result.id = s_imageHandlePool.AllocateHandle(imageResource);

        imageResource->handle = result.id;

        return result;
    }

    Sampler CreateSampler(const SamplerDescription& desc)
    {
        Sampler result = Sampler();

        SamplerResource* samplerResource = new SamplerResource(desc);
        result.id = s_samplerHandlePool.AllocateHandle(samplerResource);

        samplerResource->handle = result.id;

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

    void DestroySampler(const Sampler& sampler)
    {
        s_samplerHandlePool.FreeHandle(sampler.id);
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

    void UpdateImageMemory(Image image, void* data, size_t size)
    {
        ImageResource* imageResource = s_imageHandlePool.FetchResource(image.id);


        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferDeviceMemory;

        CreateVulkanBuffer(imageResource->m_memSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferDeviceMemory);

        auto mapMemoryResult = s_device.mapMemory(stagingBufferDeviceMemory, 0, imageResource->m_memSize);
        VK_ASSERT(mapMemoryResult);
        memcpy(mapMemoryResult.value, data, size);
        s_device.unmapMemory(stagingBufferDeviceMemory);

        TransitionImageLayout(imageResource->m_image, imageResource->m_format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, imageResource->m_layerCount);
        CopyBufferToImage(stagingBuffer, imageResource->m_image, imageResource->m_width, imageResource->m_height, imageResource->m_layerCount);
        TransitionImageLayout(imageResource->m_image, imageResource->m_format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, imageResource->m_layerCount);

        s_device.freeMemory(stagingBufferDeviceMemory);
        s_device.destroyBuffer(stagingBuffer);
    }

    void CopyBufferToImage(Image image, Buffer buffer)
    {
        ImageResource* imageResource = s_imageHandlePool.FetchResource(image.id);
        BufferResource* bufferResource = s_bufferHandlePool.FetchResource(buffer.id);

        TransitionImageLayout(imageResource->m_image, imageResource->m_format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, imageResource->m_layerCount);
        CopyBufferToImage(bufferResource->m_buffer, imageResource->m_image, imageResource->m_width, imageResource->m_height, imageResource->m_layerCount);
        TransitionImageLayout(imageResource->m_image, imageResource->m_format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, imageResource->m_layerCount);
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

        if (descriptorSets.size() > 0)
        {
            s_commandBuffersDefault[s_currentImageIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, s_currentPipleline->m_pipelineLayout, 0, descriptorSets, nullptr);
        }

        s_commandBuffersDefault[s_currentImageIndex].draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
    {
        std::vector<vk::DescriptorSet> descriptorSets;
        for (int i = 0; i < s_currentDescriptors.size(); i++)
        {
            descriptorSets.push_back(s_currentDescriptors[i]);
        }

        if (descriptorSets.size() > 0)
        {
            s_commandBuffersDefault[s_currentImageIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, s_currentPipleline->m_pipelineLayout, 0, descriptorSets, nullptr);
        }

        s_commandBuffersDefault[s_currentImageIndex].drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void SetViewport(float x, float y, float w, float h)
    {
        vk::Viewport newViewport = {};
        newViewport.setX(x);
        newViewport.setY(y);
        newViewport.setWidth(w);
        newViewport.setHeight(h);
        newViewport.setMinDepth(0.0f);
        newViewport.setMaxDepth(1.0f);

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
        std::map<const char*, const char*> instanceExtensions;
        std::map<const char*, const char*> deviceExtensions;

        for (auto extensionName : s_expectedExtensions)
        {
            deviceExtensions[extensionName] = extensionName;
        }

        for (auto extension : desc.extensions)
        {
            if (extension == GFX::Extension::Raytracing)
            {
                instanceExtensions[VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
                deviceExtensions[VK_KHR_RAY_TRACING_EXTENSION_NAME] = VK_KHR_RAY_TRACING_EXTENSION_NAME;
                deviceExtensions[VK_KHR_MAINTENANCE3_EXTENSION_NAME] = VK_KHR_MAINTENANCE3_EXTENSION_NAME;
                deviceExtensions[VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME] = VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME;
                deviceExtensions[VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME] = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
                deviceExtensions[VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME] = VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME;
            }
        }

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
        for (int i = 0; i < glfwExtensionCount; i++)
        {
            instanceExtensions[glfwExtensions[i]] = glfwExtensions[i];
        }
        
        std::vector<const char*> finalInstanceExtensions;
        for (auto& pair : instanceExtensions)
        {
            finalInstanceExtensions.push_back(pair.second);
        }

        createInfo.setEnabledExtensionCount(finalInstanceExtensions.size());
        createInfo.setPpEnabledExtensionNames(finalInstanceExtensions.data());

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

        // Features
        vk::PhysicalDeviceFeatures deviceFeatures = s_physicalDevice.getFeatures();

        // Device Create Info
        vk::DeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.setQueueCreateInfoCount(queueCreateInfos.size());
        deviceCreateInfo.setPQueueCreateInfos(queueCreateInfos.data());

        for (auto& pair : deviceExtensions)
        {
            s_expectedExtensions.push_back(pair.second);
        }

        deviceCreateInfo.setEnabledExtensionCount(s_expectedExtensions.size());
        deviceCreateInfo.setPpEnabledExtensionNames(s_expectedExtensions.data());
        deviceCreateInfo.setPEnabledFeatures(&deviceFeatures);

        auto allDeviceExtensions = s_physicalDevice.enumerateDeviceExtensionProperties();

        auto createDeviceResult = s_physicalDevice.createDevice(deviceCreateInfo);
        VK_ASSERT(createDeviceResult);
        s_device = createDeviceResult.value;

        // Create Default Queue
        s_graphicsQueueDefault = s_device.getQueue(s_graphicsFamily, 0);
        s_presentQueueDefault = s_device.getQueue(s_presentFamily, 0);

        CreateSwapChain();
        CreateImageViews();
     
        CreateCommandPoolDefault();

        ktxVulkanDeviceInfo_Construct(&s_ktx_device_info, s_physicalDevice, s_device, s_graphicsQueueDefault, s_commandPoolDefault, nullptr);

        CreateCommandBuffersDefault();
        CreateSyncObjects();
        CreateDescriptorPoolDefault();

        for (auto extension : desc.extensions)
        {
            if (extension == GFX::Extension::Raytracing)
            {
                auto props = s_physicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPropertiesKHR>();
                s_rayTracingProperties = props.get<vk::PhysicalDeviceRayTracingPropertiesKHR>();
            }
        }
    }

    void Resize(int width, int height)
    {
        RecreateSwapChain();
    }

    // resize all attachments
    void ResizeRenderPass(RenderPass renderPass, int width, int height)
    {
        RenderPassResource* renderPassResource = s_renderPassHandlePool.FetchResource(renderPass.id);
        renderPassResource->Resize(width, height);
    }

    bool BeginFrame()
    {
        s_device.waitForFences(1, &s_inFlightFences[s_currentFrame], false, UINT64_MAX);
        s_device.resetFences(1, &s_inFlightFences[s_currentFrame]);

        auto acquireNextImageResult = s_device.acquireNextImageKHR(s_swapChain, UINT64_MAX, s_imageAvailableSemaphores[s_currentFrame], nullptr);
        
        if (acquireNextImageResult.result == vk::Result::eErrorOutOfDateKHR || acquireNextImageResult.result == vk::Result::eSuboptimalKHR)
        {
            printf("Inline Resize \n");
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

    /*void BeginDefaultRenderPass()
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

        vk::ClearValue clearDepthStencil = {};
        clearDepthStencil.setDepthStencil({ 1.0f, 0 });

        std::vector<vk::ClearValue> clearValues = { clearColor, clearDepthStencil };

        renderPassBeginInfo.setClearValueCount(clearValues.size());
        renderPassBeginInfo.setPClearValues(clearValues.data());
        
        s_commandBuffersDefault[s_currentImageIndex].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    }*/

    void BeginRenderPass(RenderPass renderPass, int offsetX, int offsetY, int width, int height)
    {
        auto renderPassResource = s_renderPassHandlePool.FetchResource(renderPass.id);
        
        vk::RenderPassBeginInfo renderPassBeginInfo = {};

        renderPassBeginInfo.setRenderPass(renderPassResource->m_renderPass);
        renderPassBeginInfo.setFramebuffer(renderPassResource->m_framebuffers[s_currentImageIndex]);
        renderPassBeginInfo.setClearValueCount(renderPassResource->m_clearValues.size());
        renderPassBeginInfo.setPClearValues(renderPassResource->m_clearValues.data());

        vk::Rect2D rect = {};
        rect.setOffset({ offsetX, offsetY });
        rect.setExtent({ (uint32_t)width, (uint32_t)height });
        renderPassBeginInfo.setRenderArea(rect);

        s_commandBuffersDefault[s_currentImageIndex].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    }

    void NextSubpass()
    {
        s_commandBuffersDefault[s_currentImageIndex].nextSubpass(vk::SubpassContents::eInline);
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
            printf("Inline Resize \n");
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

        ktxVulkanDeviceInfo_Destruct(&s_ktx_device_info);

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
            s_swapChainImageViews.push_back(CreateVulkanImageView(s_swapChainImages[i], s_swapChainImageFormat, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::e2D, 1, 1));
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

    void RecreateSwapChain()
    {
        s_device.waitIdle();

        /*for (auto framebuffer : s_swapChainFrameBuffers)
        {
            vkDestroyFramebuffer(s_device, framebuffer, nullptr);
        }*/

        for (auto imageView : s_swapChainImageViews)
        {
            vkDestroyImageView(s_device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(s_device, s_swapChain, nullptr);

        // s_swapChainFrameBuffers.clear();
        s_swapChainImageViews.clear();
        s_swapChainImages.clear();

        // s_device.destroyImageView(s_depthImageView);
       /* s_device.destroyImage(s_depthImage);
        s_device.freeMemory(s_depthImageMemory);*/

        CreateSwapChain();
        CreateImageViews();
        // CreateDepthImage();
        // CreateSwapChainFramebuffers();
    }

  /*  void CreateSwapChainFramebuffers()
    {
        s_swapChainFrameBuffers.resize(s_swapChainImageViews.size());
        for (size_t i = 0; i < s_swapChainImageViews.size(); i++)
        {
            std::vector<VkImageView> attachments = { s_swapChainImageViews[i], s_depthImageView };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = s_renderPassDefault;
            framebufferInfo.attachmentCount = attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = s_swapChainImageExtent.width;
            framebufferInfo.height = s_swapChainImageExtent.height;
            framebufferInfo.layers = 1;

            auto result = vkCreateFramebuffer(s_device, &framebufferInfo, nullptr, &s_swapChainFrameBuffers[i]);
            assert(result == VK_SUCCESS);
        }
    }*/

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

        vk::DescriptorPoolSize inputAttachmentPoolSize = {};
        inputAttachmentPoolSize.setType(vk::DescriptorType::eInputAttachment);
        inputAttachmentPoolSize.setDescriptorCount(30);

        vk::DescriptorPoolSize combinedImageSamplerPoolSize = {};
        combinedImageSamplerPoolSize.setType(vk::DescriptorType::eCombinedImageSampler);
        combinedImageSamplerPoolSize.setDescriptorCount(200);

        std::vector<vk::DescriptorPoolSize> sizes;
        sizes.push_back(uniformmBufferPoolSize);
        sizes.push_back(texturePoolSize);
        sizes.push_back(inputAttachmentPoolSize);
        sizes.push_back(combinedImageSamplerPoolSize);

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
        case ShaderStage::AnyHit:
            return vk::ShaderStageFlagBits::eAnyHitKHR;
        case ShaderStage::ClosetHit:
            return vk::ShaderStageFlagBits::eClosestHitKHR;
        case ShaderStage::Intersection:
            return vk::ShaderStageFlagBits::eIntersectionKHR;
        case ShaderStage::Miss:
            return vk::ShaderStageFlagBits::eMissKHR;
        case ShaderStage::RayGen:
            return vk::ShaderStageFlagBits::eRaygenKHR;
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
        case UniformType::SampledImage:
            return vk::DescriptorType::eCombinedImageSampler;
        case UniformType::InputAttachment:
            return vk::DescriptorType::eInputAttachment;
        }
    }

    vk::Filter MapFilterForVulkan(const FilterMode& filterMode)
    {
        switch (filterMode)
        {
        case FilterMode::Linear:
            return vk::Filter::eLinear;
        case FilterMode::Nearest:
            return vk::Filter::eNearest;
        default:
            assert(false);
            return vk::Filter::eCubicIMG;
        }
    }

    vk::SamplerMipmapMode MapMipmapFilterForVulkan(const FilterMode& filterMode)
    {
        switch (filterMode)
        {
        case FilterMode::Linear:
            return vk::SamplerMipmapMode::eLinear;
        case FilterMode::Nearest:
            return vk::SamplerMipmapMode::eNearest;
        default:
            assert(false);
            return vk::SamplerMipmapMode::eLinear;
        }
    }

    vk::SamplerAddressMode MapWrapModeForVulkan(const WrapMode& wrapMode)
    {
        switch (wrapMode)
        {
        case WrapMode::Repeat:
            return vk::SamplerAddressMode::eRepeat;
        case WrapMode::ClampToEdge:
            return vk::SamplerAddressMode::eClampToEdge;
        case WrapMode::ClampToBorder:
            return vk::SamplerAddressMode::eClampToBorder;
        case WrapMode::MirroredClampToEdge:
            return vk::SamplerAddressMode::eMirrorClampToEdge;
        case WrapMode::MirroredRepeat:
            return vk::SamplerAddressMode::eMirroredRepeat;
        default:
            assert(false);
            return vk::SamplerAddressMode::eClampToBorder;
        }
    }

    vk::BorderColor MapBorderColorForVulkan(const BorderColor& borderColor)
    {
        switch (borderColor)
        {
        case BorderColor::FloatOpaqueBlack:
            return vk::BorderColor::eFloatOpaqueBlack;
        case BorderColor::FloatTransparentBlack:
            return vk::BorderColor::eFloatTransparentBlack;
        case BorderColor::IntOpaqueBlack:
            return vk::BorderColor::eIntOpaqueBlack;
        case BorderColor::IntTransparentBlack:
            return vk::BorderColor::eIntTransparentBlack;
        case BorderColor::FloatOpaqueWhite:
            return vk::BorderColor::eFloatOpaqueWhite;
        case BorderColor::IntOpaqueWhite:
            return vk::BorderColor::eIntOpaqueWhite;
        }
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
        case Format::SWAPCHAIN:
            return s_swapChainImageFormat;
        case Format::DEPTH_16UNORM_STENCIL_8INT:
            return vk::Format::eD16UnormS8Uint;
        case Format::DEPTH_24UNORM_STENCIL_8INT:
            return vk::Format::eD24UnormS8Uint;
        case Format::DEPTH_32FLOAT:
            return vk::Format::eD32Sfloat;
        case Format::DEPTH:
            return FindDepthFormat();
        default:
            assert(false);
            return vk::Format::eA1R5G5B5UnormPack16;
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

    vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tilling, vk::FormatFeatureFlags features)
    {
        assert(candidates.size() > 0);

        for (auto candidate : candidates)
        {
            vk::FormatProperties props = s_physicalDevice.getFormatProperties(candidate);
            if (tilling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
            {
                return candidate;
            }
            else if (tilling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
            {
                return candidate;
            }
        }

        assert(false);
        return candidates[0];
    }

    vk::Format FindDepthFormat()
    {
        return FindSupportedFormat({ vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16UnormS8Uint, vk::Format::eD32SfloatS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    bool HasStencilComponent(vk::Format format)
    {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint || format ==  vk::Format::eD16UnormS8Uint;
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

    void TransitionImageLayout(vk::Image img, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t layerCount)
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
        subresourceRange.setLayerCount(layerCount);
        
        if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
            if (HasStencilComponent(format))
            {
                subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
            }
        }
        else
        {
            subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
        }

        barrier.setSubresourceRange(subresourceRange);

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) 
        {
            barrier.setSrcAccessMask({});
            barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) 
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
            barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            barrier.setSrcAccessMask({});
            barrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        }
        else 
        {
            assert(false);
        }

        cb.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);

        EndOneTimeCommandBuffer(cb);
    }

    void CopyBufferToImage(vk::Buffer buffer, vk::Image img, uint32_t width, uint32_t height, uint32_t layerCount)
    {
        auto cb = BeginOneTimeCommandBuffer();

        vk::BufferImageCopy bufferImageCopy = {};
        bufferImageCopy.setBufferOffset(0);
        bufferImageCopy.setBufferImageHeight(0);
        bufferImageCopy.setBufferRowLength(0);

        bufferImageCopy.setImageExtent({ width, height, 1 });
        bufferImageCopy.setImageOffset({ 0, 0, 0 });

        vk::ImageSubresourceLayers subresourceLayer = {};
        subresourceLayer.setBaseArrayLayer(0);
        subresourceLayer.setLayerCount(layerCount);
        subresourceLayer.setAspectMask(vk::ImageAspectFlagBits::eColor);
        subresourceLayer.setMipLevel(0);
        bufferImageCopy.setImageSubresource(subresourceLayer);

        cb.copyBufferToImage(buffer, img, vk::ImageLayout::eTransferDstOptimal, bufferImageCopy);

        EndOneTimeCommandBuffer(cb);
    }

    void CreateVulkanImage(uint32_t width, uint32_t height, 
        vk::Format format, 
        vk::ImageTiling tiling, 
        vk::ImageUsageFlags usage, 
        vk::MemoryPropertyFlags properties, 
        vk::Image& image, 
        vk::DeviceMemory& imageMemory)
    {
        vk::ImageCreateInfo imageInfo = {};
        imageInfo.setImageType(vk::ImageType::e2D);
        imageInfo.setExtent({ width, height, 1});
        imageInfo.setMipLevels(1);
        imageInfo.setArrayLayers(1);
        imageInfo.setFormat(format);
        imageInfo.setTiling(tiling);
        imageInfo.setInitialLayout(vk::ImageLayout::eUndefined);
        imageInfo.setUsage(usage);
        imageInfo.setSamples(vk::SampleCountFlagBits::e1);
        imageInfo.setSharingMode(vk::SharingMode::eExclusive);

        auto createImageResult = s_device.createImage(imageInfo);
        VK_ASSERT(createImageResult);
        image = createImageResult.value;

        vk::MemoryRequirements memReq = s_device.getImageMemoryRequirements(image);

        vk::MemoryAllocateInfo memAllocInfo = {};
        memAllocInfo.setAllocationSize(memReq.size);
        memAllocInfo.setMemoryTypeIndex(FindMemoryType(memReq.memoryTypeBits, properties));

        auto allocateMemoryResult = s_device.allocateMemory(memAllocInfo);
        VK_ASSERT(allocateMemoryResult);
        imageMemory = allocateMemoryResult.value;

        s_device.bindImageMemory(image, imageMemory, 0);
    }

    vk::ImageView CreateVulkanImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect, vk::ImageViewType type, uint32_t layerCount, uint32_t levelCount) 
    {
        vk::ImageViewCreateInfo viewInfo = {};
        viewInfo.setImage(image);
        viewInfo.setViewType(type);
        viewInfo.setFormat(format);

        vk::ImageSubresourceRange imageSubresourceRange = {};
        imageSubresourceRange.setAspectMask(aspect);
        imageSubresourceRange.setBaseMipLevel(0);
        imageSubresourceRange.setLevelCount(levelCount);
        imageSubresourceRange.setBaseArrayLayer(0);
        imageSubresourceRange.setLayerCount(layerCount);
        viewInfo.setSubresourceRange(imageSubresourceRange);

        auto createImageViewResult = s_device.createImageView(viewInfo);
        VK_ASSERT(createImageViewResult);

        return createImageViewResult.value;
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