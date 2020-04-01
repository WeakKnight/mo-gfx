#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerHdr;
// layout (input_attachment_index = 1, binding = 0) uniform subpassInput samplerHdr;

layout (location = 0) out vec4 outColor;

void main()
{
    // outColor = vec4(subpassLoad(samplerHdr).rgb, 1.0) + vec4(1.0, 0.0, 0.0, 1.0);
    outColor = vec4(texture(samplerHdr, inUV).rgb, 1.0) + vec4(0.3, 0.0, 0.0, 1.0);
}