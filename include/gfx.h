#pragma once

#include <string>
#include <vector>
#include <memory>
#include <list>
#include <cassert>

#include "ktx.h"
#include "ktxvulkan.h"

struct GLFWwindow;

namespace GFX
{
    template<typename T>
    class HandlePool
    {
    public:
        HandlePool(size_t reservedSize = 200)
        {
            m_handles.reserve(reservedSize);
            m_resources.reserve(reservedSize);
        }

        uint32_t AllocateHandle(T* resource)
        {
            if (m_avaliableHandleList.size() != 0)
            {
                uint32_t result = m_avaliableHandleList.back();
                m_avaliableHandleList.pop_back();

                m_handles[result] = result;

                m_resources[result] = resource;

                return result;
            }
            else
            {
                uint32_t result = m_handles.size();
                m_handles.push_back(result);
                m_resources.push_back(resource);

                return result;
            }
        }

        void FreeHandle(uint32_t handle)
        {
            m_avaliableHandleList.push_back(handle);
            delete m_resources[handle];
            m_resources[handle] = nullptr;
        }

        T* FetchResource(uint32_t handle)
        {
            return m_resources[handle];
        }

    private:
        std::vector<uint32_t> m_handles;
        std::vector<T*> m_resources;
        std::list<uint32_t> m_avaliableHandleList;
    };

    enum class Extension
    {
        Raytracing
    };

    struct InitialDescription
    {
        bool debugMode = false;
        std::vector<Extension> extensions;
        GLFWwindow* window = nullptr;
    };

    struct Color
    {
        Color(float r, float g, float b, float a)
        {
            this->r = r;
            this->g = g;
            this->b = b;
            this->a = a;
        }

        Color()
        {
            r = 0.0f;
            g = 0.0f;
            b = 0.0f;
            a = 0.0f;
        }

        float r, g, b, a;
    };

    struct Vec2
    {
        Vec2(float x, float y)
        {
            this->x = x;
            this->y = y;
        }

        Vec2()
        {
            this->x = 0.0f;
            this->y = 0.0f;
        }

        float x, y;
    };

    enum class PipelineType
    {
        Compute,
        Graphics,
        RayTracing
    };
    
    /*
    U: Unsigned S: Signed F: Float I: Int
    */
    enum class Format
    {
        R32SF,
        R8G8B8A8,
        R8G8B8,
        R16G16B16A16F,
        R16G16B16F,
        R32G32B32A32F,
        R32G32B32F,
        DEPTH_24UNORM_STENCIL_8INT,
        DEPTH_16UNORM_STENCIL_8INT,
        DEPTH_32FLOAT,
        /*
        Find Usable Depth Format Automaticly
        */
        DEPTH,
        SWAPCHAIN
    };

    enum class BufferUsage
    {
        VertexBuffer,
        UniformBuffer,
        IndexBuffer,
        TransferBuffer
    };

    enum class BufferStorageMode
    {
        /*
        Host Coherent
        */
        Dynamic,
        /*
        Device Local,
        */
        Static,
    };

    struct BufferDescription
    {
        BufferUsage usage;
        BufferStorageMode storageMode;
        size_t size = 0;
    };

    struct Buffer
    {
        uint32_t id = 0;
    };

    enum class FrontFace
    {
        Clockwise,
        CounterClockwise
    };

    enum class CullFace
    {
        Front,
        Back,
        FrontAndBack,
        None
    };

    enum class ImageType
    {
        Image2D,
        Cube
    };

    enum class ImageUsage
    {
        SampledImage,
        ColorAttachment,
        DepthStencilAttachment,
    };

    enum class ImageSampleCount
    {
        Sample1,
        Sample2,
        Sample4
    };

    enum class ImageLayout
    {
        ColorAttachment,
        FragmentShaderRead,
        General,
        DepthStencilAttachment,
        Undefined,
        Present
    };

    struct ImageDescription
    {
        uint32_t width;
        uint32_t height;
        uint32_t depth = 1;
        uint32_t mipLevels = 1;
        ImageType type;
        ImageUsage usage;
        Format format;
        ImageSampleCount sampleCount = ImageSampleCount::Sample1;
        bool readOrWriteByCPU = false;
    };

    struct Image
    {
        uint32_t id = 0;
    };

    enum class FilterMode
    {
        Linear,
        Nearest
    };

    enum class WrapMode
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        MirroredClampToEdge,
        ClampToBorder,
    };

    enum class BorderColor
    {
        IntOpaqueBlack,
        IntTransparentBlack,
        FloatOpaqueBlack,
        FloatTransparentBlack,
        IntOpaqueWhite,
        FloatOpaqueWhite,
    };

    struct SamplerDescription
    {
        FilterMode minFilter = FilterMode::Linear;
        FilterMode magFilter = FilterMode::Linear;
        FilterMode mipmapFilter = FilterMode::Linear;

        WrapMode wrapU = WrapMode::ClampToEdge;
        WrapMode wrapV = WrapMode::ClampToEdge;
        WrapMode wrapW = WrapMode::ClampToEdge;
        
        float maxLod = 10.0f;

        bool anisotropyEnable = false;
        int maxAnisotropy = 16;
        BorderColor borderColor = BorderColor::FloatOpaqueBlack;
        bool normalizedCoordinates = true;
    };

    struct Sampler
    {
        uint32_t id = 0;
    };

    enum class ShaderStage
    {
        Vertex,
        Fragment,
        Compute,
        ClosetHit,
        Miss,
        RayGen,
        Intersection,
        AnyHit,
        None,
        VertexFragment
    };

    struct ShaderDescription
    {
        ShaderStage stage = ShaderStage::None;
        std::string codes = "";
        std::string name = "";
    };

    struct Shader
    {
        uint32_t id = 0;
    };

    enum class PrimitiveTopology
    {
        TriangleList,
        TriangleStrip,
        LineList,
        LineStrip,
        PointList
    };

    enum class AttachmentType
    {
        Color,
        DepthStencil,
        Present
    };

    enum class AttachmentLoadAction
    {
        Clear,
        Load,
        DontCare,
    };

    enum class AttachmentStoreAction
    {
        Store,
        DontCare,
    };

    struct ClearValue
    {
        void SetColor(Color& color)
        {
            this->color = color;
            hasColor = true;
            assert(!hasDepthStencil);
        }

        void SetDepth(float depth)
        {
            this->depth = depth;
            hasDepthStencil = true;
            assert(!hasColor);
        }

        void SetStencil(float stencil)
        {
            this->stencil = stencil;
            hasDepthStencil = true;
            assert(!hasColor);
        }

        Color color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        float depth = 1.0f;
        uint32_t stencil = 0;

        bool hasDepthStencil = false;
        bool hasColor = false;
    };

    struct AttachmentDescription
    {
        Format format;
        ImageSampleCount samples = ImageSampleCount::Sample1;
        uint32_t width;
        uint32_t height;
        AttachmentLoadAction loadAction = AttachmentLoadAction::DontCare;
        AttachmentLoadAction stencilLoadAction = AttachmentLoadAction::DontCare;
        AttachmentStoreAction storeAction = AttachmentStoreAction::DontCare;
        AttachmentStoreAction stencilStoreAction = AttachmentStoreAction::DontCare;
        ClearValue clearValue;
        AttachmentType type;
        ImageLayout initialLayout = ImageLayout::Undefined;
        ImageLayout finalLayout = ImageLayout::Undefined;
    };

    struct SubPassDescription
    {
        PipelineType pipelineType;
        std::vector<uint32_t> colorAttachments;
        uint32_t depthStencilAttachment;
        bool hasDepth = false;
        std::vector<uint32_t> inputAttachments;

        void SetDepthStencilAttachment(uint32_t index)
        {
            depthStencilAttachment = index;
            hasDepth = true;
        }
    };

    enum class PipelineStage
    {
        ColorAttachmentOutput,
        FragmentShader,
        VertexShader,
        LateFragmentTests,
        EarlyFragmentTests,
        All
    };

    enum class Access
    {
        ColorAttachmentWrite,
        DepthStencilAttachmentWrite,
        ShaderRead,
        InputAttachmentRead,
    };

    constexpr auto ExternalSubpass = (~0U);

    struct DependencyDescription
    {
        uint32_t srcSubpass;
        uint32_t dstSubpass;
        PipelineStage srcStage;
        PipelineStage dstStage;
        Access srcAccess;
        Access dstAccess;
    };

    struct RenderPassDescription
    {
        uint32_t width;
        uint32_t height;
        std::vector<AttachmentDescription> attachments;
        std::vector<SubPassDescription> subpasses;
        std::vector<DependencyDescription> dependencies;
    };

    struct RenderPass
    {
        uint32_t id;
    };

    enum class IndexType
    {
        UInt16,
        UInt32,
    };

    enum class ValueType
    {
        Float32x2,
        Float32x3,
        UInt16
    };

    enum class BindingType
    {
        Vertex,
        Instance
    };

    struct VertexBindings
    {
        struct AttributeDescription
        {
            uint32_t location = 0;
            ValueType type;
            size_t offset = 0;
        };

        void AddAttribute(uint32_t location, size_t offset, ValueType type)
        {
            AttributeDescription desc = {};
            desc.offset = offset;
            desc.type = type;
            desc.location = location;

            m_layout.push_back(desc);
        }

        void SetStrideSize(size_t size)
        {
            m_strideSize = size;
        }

        void SetBindingType(BindingType bindingType)
        {
            m_bindingType = bindingType;
        }

        void SetBindingPosition(uint32_t pos)
        {
            m_bindingPosition = pos;
        }

        BindingType m_bindingType = BindingType::Vertex;
        size_t m_strideSize = 0;
        std::vector<AttributeDescription> m_layout;
        uint32_t m_bindingPosition = 0;
    };

    enum class UniformType
    {
        UniformBuffer,
        SampledImage,
        InputAttachment,
    };

    struct UniformLayoutDescription
    {
        struct UniformBinding
        {
            uint32_t binding = 0;
            UniformType type;
            ShaderStage stage;
            uint32_t count;
        };

        void AddUniformBinding(uint32_t binding, UniformType type, ShaderStage stage, uint32_t count)
        {
            UniformBinding desc = {};
            desc.binding = binding;
            desc.type = type;
            desc.stage = stage;
            desc.count = count;

            m_layout.push_back(desc);
        }

        std::vector<UniformBinding> m_layout;
    };

    struct UniformLayout
    {
        uint32_t id = 0;
    };

    struct UniformBindings
    {
        void AddUniformLayout(UniformLayout layout)
        {
            m_layouts.push_back(layout);
        }

        std::vector<UniformLayout> m_layouts;
    };

    enum class UniformStorageMode
    {
        Dynamic,
        Static
    };

    struct UniformBufferAtrribute
    {
        uint32_t binding;
        Buffer buffer;
        size_t offset;
        size_t range;
    };

    struct UniformImageAttribute
    {
        uint32_t binding;
        Image image;
        Sampler sampler;
    };

    struct UniformInputAttachmentAttribute
    {
        uint32_t binding;
        RenderPass renderPass;
        uint32_t attachmentIndex;
    };

    struct UniformSampledAttachmentAttribute
    {
        uint32_t binding;
        RenderPass renderPass;
        uint32_t attachmentIndex;
        Sampler sampler;
    };

    struct UniformDescription
    {
        void AddBufferAttribute(uint32_t binding, Buffer buffer, size_t offset, size_t range)
        {
            UniformBufferAtrribute attr = {};
            attr.binding = binding;
            attr.buffer = buffer;
            attr.offset = offset;
            attr.range = range;

            m_bufferAtrributes.push_back(attr);
        }

        void AddImageAttribute(uint32_t binding, Image image, Sampler sampler)
        {
            UniformImageAttribute attr = {};
            attr.binding = binding;
            attr.image = image;
            attr.sampler = sampler;

            m_imageAttributes.push_back(attr);
        }

        void AddInputAttachmentAttribute(uint32_t binding, RenderPass renderPass, uint32_t attachmentIndex)
        {
            UniformInputAttachmentAttribute attr = {};
            attr.binding = binding;
            attr.renderPass = renderPass;
            attr.attachmentIndex = attachmentIndex;

            m_inputAttachmentAttributes.push_back(attr);
        }

        void AddSampledAttachmentAttribute(uint32_t binding, RenderPass renderPass, uint32_t attachmentIndex, Sampler sampler)
        {
            UniformSampledAttachmentAttribute attr = {};
            attr.binding = binding;
            attr.renderPass = renderPass;
            attr.attachmentIndex = attachmentIndex;
            attr.sampler = sampler;

            m_sampledAttachmentAttributes.push_back(attr);
        }

        void SetUniformLayout(UniformLayout layout)
        {
            m_layout = layout;
        }

        void SetStorageMode(UniformStorageMode storageMode)
        {
            m_storageMode = storageMode;
        }

        UniformLayout m_layout;
        UniformStorageMode m_storageMode;
        std::vector<UniformBufferAtrribute> m_bufferAtrributes;
        std::vector<UniformImageAttribute> m_imageAttributes;
        std::vector<UniformInputAttachmentAttribute> m_inputAttachmentAttributes;
        std::vector<UniformSampledAttachmentAttribute> m_sampledAttachmentAttributes;
    };

    struct Uniform
    {
        uint32_t id = 0;
    };

    struct BlendState
    {
        bool enable = false;
    };

    struct GraphicsPipelineDescription
    {
        std::vector<Shader> shaders;
        PrimitiveTopology primitiveTopology;
        VertexBindings vertexBindings;
        UniformBindings uniformBindings;
        RenderPass renderPass;
        bool enableDepthTest = false;
        bool enableStencilTest = false;
        uint32_t subpass = 0;
        FrontFace fronFace = FrontFace::CounterClockwise;
        CullFace cullFace = CullFace::Back;
        std::vector<BlendState> blendStates;
    };

    struct Pipeline
    {
        uint32_t id = 0;
    };

    void Init(const InitialDescription& desc);

    Pipeline CreatePipeline(const GraphicsPipelineDescription& desc);
    Shader CreateShader(const ShaderDescription& desc);
    RenderPass CreateRenderPass(const RenderPassDescription& desc);
    Buffer CreateBuffer(const BufferDescription& desc);
    Image CreateImage(const ImageDescription& desc);
    Sampler CreateSampler(const SamplerDescription& desc);
    UniformLayout CreateUniformLayout(const UniformLayoutDescription& desc);
    Uniform CreateUniform(const UniformDescription& desc);

    Image CreateImageFromKtxTexture(const char* path);
    
    void DestroyShader(const Shader& shader);
    void DestroyPipeline(const Pipeline& pipeline);
    void DestroyRenderPass(const RenderPass& renderPass);
    void DestroyBuffer(const Buffer& buffer);
    void DestroyImage(const Image& image);
    void DestroySampler(const Sampler& sampler);
    void DestroyUniformLayout(const UniformLayout& uniformLayout);
    void DestroyUniform(const Uniform& uniform);

    /*
    Buffer Operation
    */

    size_t GetMinimumUniformBufferAlignment();

    size_t AlignmentSize(size_t size, size_t alignment);
    size_t UniformAlign(size_t size);

    void UpdateBuffer(Buffer buffer, size_t offset, size_t size, void* data);

    /*
    Image Operation
    */
    void UpdateImageMemory(Image image, void* data, size_t size);
    void CopyBufferToImage(Image image, Buffer buffer);
    void AttachmentLayoutTransition(RenderPass renderPass, uint32_t attachmentIndex, ImageLayout oldLayout, ImageLayout newLayout);

    /*
    Uniforms
    */
    void BindUniform(Uniform uniform, uint32_t set);
    void UpdateUniformBuffer(Uniform uniform, uint32_t binding, void* data);

    /*
    Rendering Operation
    */

    void Resize(int width, int height);
    void ResizeRenderPass(RenderPass renderPass, int width, int height);

    bool BeginFrame();
    void ApplyPipeline(Pipeline pipeline);
    void BindIndexBuffer(Buffer buffer, size_t offset, IndexType indexType);
    void BindVertexBuffer(Buffer buffer, size_t offset, uint32_t binding = 0);
    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset = 0, uint32_t firstInstance = 0);
    void SetViewport(float x, float y, float w, float h);
    void SetScissor(float x, float y, float w, float h);

    void BeginRenderPass(RenderPass renderPass, int offsetX, int offsetY, int width, int height);
    void NextSubpass();
    // void BeginDefaultRenderPass();
    void EndRenderPass();

    void EndFrame();

    void Shutdown();
};