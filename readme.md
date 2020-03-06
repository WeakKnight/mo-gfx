# mo-gfx an api agnostic graphics api(WIP)

## Init
```c++
GFX::InitialDescription initDesc = {};
initDesc.debugMode = true;
initDesc.window = m_window;

GFX::Init(initDesc);

GFX::BufferDescription vertexBufferDescription = {};
vertexBufferDescription.size = sizeof(Vertex) * vertices.size();
vertexBufferDescription.storageMode = GFX::BufferStorageMode::Static;
vertexBufferDescription.usage = GFX::BufferUsage::VertexBuffer;

vertexBuffer = GFX::CreateBuffer(vertexBufferDescription);
GFX::UpdateBuffer(vertexBuffer, 0, vertexBufferDescription.size, (void*)vertices.data());

GFX::BufferDescription indexBufferDescription = {};
indexBufferDescription.size = sizeof(uint16_t) * indices.size();
indexBufferDescription.storageMode = GFX::BufferStorageMode::Static;
indexBufferDescription.usage = GFX::BufferUsage::IndexBuffer;

indexBuffer = GFX::CreateBuffer(indexBufferDescription);
GFX::UpdateBuffer(indexBuffer, 0, indexBufferDescription.size, (void*)indices.data());

GFX::BufferDescription uniformBufferDescription = {};
uniformBufferDescription.size = GFX::AlignmentSize(sizeof(UniformBufferObject), GFX::GetMinimumUniformBufferAlignment()) + GFX::AlignmentSize(sizeof(float), GFX::GetMinimumUniformBufferAlignment());
int a = sizeof(UniformBufferObject);
int b = GFX::AlignmentSize(sizeof(UniformBufferObject), GFX::GetMinimumUniformBufferAlignment());
int c = sizeof(float);
int d = GFX::AlignmentSize(sizeof(float), GFX::GetMinimumUniformBufferAlignment());

uniformBufferDescription.storageMode = GFX::BufferStorageMode::Dynamic;
uniformBufferDescription.usage = GFX::BufferUsage::UniformBuffer;

uniformBuffer = GFX::CreateBuffer(uniformBufferDescription);

GFX::ShaderDescription vertDesc = {};
vertDesc.name = "default";
vertDesc.codes = StringUtils::ReadFile("default.vert");
vertDesc.stage = GFX::ShaderStage::Vertex;

GFX::ShaderDescription fragDesc = {};
fragDesc.name = "default";
fragDesc.codes = StringUtils::ReadFile("default.frag");
fragDesc.stage = GFX::ShaderStage::Fragment;

vertShader = GFX::CreateShader(vertDesc);
fragShader = GFX::CreateShader(fragDesc);

GFX::VertexBindings vertexBindings = {};
vertexBindings.AddAttribute(0, offsetof(Vertex, pos), GFX::ValueType::Float32x2);
vertexBindings.AddAttribute(1, offsetof(Vertex, color), GFX::ValueType::Float32x3);
vertexBindings.SetStrideSize(sizeof(Vertex));
vertexBindings.SetBindingType(GFX::BindingType::Vertex);
vertexBindings.SetBindingPosition(0);

GFX::UniformBindings uniformBindings = {};
uniformBindings.AddUniformDescription(0, GFX::UniformType::UniformBuffer, GFX::ShaderStage::Vertex, 1);
uniformBindings.AddUniformDescription(1, GFX::UniformType::UniformBuffer, GFX::ShaderStage::Vertex, 1);

GFX::GraphicsPipelineDescription pipelineDesc = {};
pipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
pipelineDesc.shaders.push_back(vertShader);
pipelineDesc.shaders.push_back(fragShader);
pipelineDesc.vertexBindings = vertexBindings;
pipelineDesc.uniformBindings = uniformBindings;

pipeline = GFX::CreatePipeline(pipelineDesc);
```

## Render
```c++
if (GFX::BeginFrame())
{
    GFX::BeginDefaultRenderPass();

    GFX::ApplyPipeline(pipeline);
    
    float time = (float)glfwGetTime();
    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), (float)s_width/(float)s_height, 0.1f, 10.0f);

    GFX::UpdateBuffer(uniformBuffer, 0, sizeof(UniformBufferObject), &ubo);
    GFX::UpdateBuffer(uniformBuffer, GFX::AlignmentSize(sizeof(UniformBufferObject), GFX::GetMinimumUniformBufferAlignment()), sizeof(float), &time);

    GFX::BindIndexBuffer(indexBuffer, 0, GFX::IndexType::UInt16);
    GFX::BindVertexBuffer(vertexBuffer, 0);
    
    GFX::SetViewport(0, 0, s_width, s_height);
    GFX::SetScissor(0, 0, s_width, s_height);
    
    GFX::BindUniform(pipeline, 0, uniformBuffer, 0, sizeof(UniformBufferObject));
    GFX::BindUniform(pipeline, 1, uniformBuffer, GFX::AlignmentSize(sizeof(UniformBufferObject), GFX::GetMinimumUniformBufferAlignment()), sizeof(float));
    GFX::DrawIndexed(indices.size(), 1, 0);

    GFX::EndRenderPass();

    GFX::EndFrame();
}
```

## Shutdown
```c++
GFX::DestroyBuffer(vertexBuffer);
GFX::DestroyBuffer(indexBuffer);
GFX::DestroyBuffer(uniformBuffer);
GFX::DestroyBuffer(uniformBuffer1);

GFX::DestroyPipeline(pipeline);

GFX::DestroyShader(vertShader);
GFX::DestroyShader(fragShader);

GFX::Shutdown();
```


## Get Started

1. Make sure you have [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) installed

2. Make sure you have python 3 installed
3. Make sure you have CMake 3.1+ installed
4. Make sure you have the latest version Visual Studio or Xcode installed
5. > python build.py
6. Project will be generated in build/macos or build/windows
7. Feel free to edit CMakeLists.txt as you want

## Thirdparty Library

Library                                     | Functionality         
------------------------------------------  | -------------
[assimp](https://github.com/assimp/assimp)  | Mesh Loading And Pre Processing
[glfw](https://github.com/glfw/glfw)        | Windowing And Input Handling
[glm](https://github.com/g-truc/glm)        | Mathematics
[imgui](https://github.com/ocornut/imgui)    | GUI
[shaderc](https://github.com/google/shaderc)  | Runtime Shader Compiler
[spdlog](https://github.com/gabime/spdlog)   | Debug Logging

