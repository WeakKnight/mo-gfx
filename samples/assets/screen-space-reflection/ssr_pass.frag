#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerNormalRoughness;
layout (binding = 1) uniform sampler2D samplerDepth;
layout (binding = 2) uniform sampler2D samplerHDR;
layout (binding = 3) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
    vec4 lightDir;
    vec4 screenSize;
    vec4 nothing1;
    vec4 nothing2;
} ubo;

layout (location = 0) out vec4 outColor;

const float PI = 3.1415927;

void main()
{
    outColor = vec4(vec3(0.3, 0.0, 0.0) * texture(samplerHDR, inUV).rgb, 1.0);
}