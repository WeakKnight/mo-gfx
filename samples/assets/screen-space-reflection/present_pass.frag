#version 450

layout (location = 0) in vec2 inUV;

// layout (input_attachment_index = 4, binding = 0) uniform subpassInput samplerHdr;
layout (binding = 0) uniform sampler2D samplerHdr;

layout (location = 0) out vec4 outColor;

void main()
{
    
    outColor = vec4(texture(samplerHdr, inUV).rgb + vec3(0.4, 0.0, 0.0), 1.0);
    // outColor = vec4(subpassLoad(samplerHdr).rgb + vec3(0.4, 0.0, 0.0), 1.0);
}