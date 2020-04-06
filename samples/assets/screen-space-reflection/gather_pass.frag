#version 450

layout (location = 0) in vec2 inUV;

layout (input_attachment_index = 0, binding = 0) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput samplerNormalRoughness;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput samplerPosition; 

layout (location = 0) out vec4 outColor;

void main()
{    
    vec4 normalRoughness = subpassLoad(samplerNormalRoughness);
    vec3 N = normalRoughness.xyz * 2.0 - vec3(1.0);

    if(length(normalRoughness.xyz) <= 0.0)
    {
        discard;
    }

    outColor = vec4(subpassLoad(samplerAlbedo).rgb, 1.0);
}