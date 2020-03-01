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

    struct Buffer
    {
        uint32_t id = 0;
    };

    struct Image
    {
        uint32_t id = 0;
    };

    enum class ShaderStage
    {
        Vertex,
        Fragment,
        Compute,
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

    struct Bindings
    {
        Buffer vertexBuffer;
        Buffer indexBuffer;
        std::vector<Image> vertexImages;
        std::vector<Image> fragImages;
    };

    struct GraphicsPipelineDescription
    {
        std::vector<Shader> shaders;
        PrimitiveTopology primitiveTopology;
        Bindings bindings;
    };

    struct Pipeline
    {
        uint32_t id = 0;
    };

    void Init(const InitialDescription& desc);

    Pipeline CreatePipeline(const GraphicsPipelineDescription& desc);
    Shader CreateShader(const ShaderDescription& desc);
    RenderPass CreateRenderPass(const RenderPassDescription& desc);
    
    void DestroyShader(const Shader& shader);
    void DestroyPipeline(const Pipeline& pipeline);
    void DestroyRenderPass(const RenderPass& renderPass);

    bool BeginFrame();
    void ApplyPipeline(Pipeline pipeline);
    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void SetViewport(float x, float y, float w, float h);
    void SetScissor(float x, float y, float w, float h);
    // void BeginDefaultCommandBuffer();
    void BeginDefaultRenderPass();
    void EndRenderPass();
    // void EndCommandBuffer();

    void Submit();

    void EndFrame();

    void Shutdown();
};