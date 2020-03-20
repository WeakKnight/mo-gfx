#pragma once

#include <string>
#include <vector>
#include <memory>
#include <list>

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

    struct InitialDescription
    {
        bool debugMode = false;
        GLFWwindow* window = nullptr;
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
    };

    enum class BufferUsage
    {
        VertexBuffer,
        UniformBuffer,
        IndexBuffer,
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

    enum class ImageType
    {
        Image2D,
    };

    enum class ImageUsage
    {
        SampledImage,
        AttachmentImage
    };

    enum class ImageSampleCount
    {
        Sample1,
        Sample2,
        Sample4
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
        WrapMode wrapU = WrapMode::ClampToEdge;
        WrapMode wrapV = WrapMode::ClampToEdge;
        WrapMode wrapW = WrapMode::ClampToEdge;
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
        All,
        None
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
        Stencil
    };

    enum class AttachmentAction
    {
        Clear,
        Store
    };

    struct AttachmentDescription
    {
        Format format;
    };

    struct RenderPassDescription
    {
        std::vector<AttachmentDescription> attachments;
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
        Sampler
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
    };

    struct Uniform
    {
        uint32_t id = 0;
    };

    struct GraphicsPipelineDescription
    {
        std::vector<Shader> shaders;
        PrimitiveTopology primitiveTopology;
        VertexBindings vertexBindings;
        UniformBindings uniformBindings;
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

    /*
    Uniforms
    */
    void BindUniform(Uniform uniform, uint32_t set);
    void UpdateUniformBuffer(Uniform uniform, uint32_t binding, void* data);

    /*
    Rendering Operation
    */
    bool BeginFrame();
    void ApplyPipeline(Pipeline pipeline);
    void BindIndexBuffer(Buffer buffer, size_t offset, IndexType indexType);
    void BindVertexBuffer(Buffer buffer, size_t offset, uint32_t binding = 0);
    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset = 0, uint32_t firstInstance = 0);
    void SetViewport(float x, float y, float w, float h);
    void SetScissor(float x, float y, float w, float h);
    void BeginDefaultRenderPass();
    void EndRenderPass();

    void EndFrame();

    void Shutdown();
};