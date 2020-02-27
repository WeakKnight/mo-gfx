#pragma once
#include "common.h"
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

    struct Bindings
    {
        Buffer vertexBuffer;
        Buffer indexBuffer;
        std::vector<Image> vertexImages;
        std::vector<Image> fragImages;
    };

    struct PipelineDescription
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

    Pipeline CreatePipeline(const PipelineDescription& desc);
    Shader CreateShader(const ShaderDescription& desc);
    void DestroyShader(const Shader& shader);

    void Submit();

    void Frame();

    void Shutdown();
};