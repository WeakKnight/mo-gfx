#version 450

layout (location = 0) in vec2 inUV;

layout (input_attachment_index = 0, binding = 0) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput samplerNormalRoughness;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput samplerPosition; 

layout(binding = 3) uniform UniformBufferObject 
{
    vec4 lightDir;
    vec4 lightColor;
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 4) uniform samplerCube skybox;
layout(binding = 5) uniform samplerCube irradianceMap;

layout (location = 0) out vec4 outColor;

void main()
{    
    vec4 normalRoughness = subpassLoad(samplerNormalRoughness);
    vec3 N = normalize(normalRoughness.xyz * 2.0 - vec3(1.0));
    vec3 L = mat3(ubo.view) * ubo.lightDir.xyz;
    mat4 viewInv = inverse(ubo.view);

    vec3 NWorld = normalize(mat3(viewInv) * N);
    vec3 radiance = texture(irradianceMap, NWorld).rgb;

    float NDotL = clamp(dot(N,L), 0.0, 1.0);

    float roughness = normalRoughness.w;

    if(length(normalRoughness.xyz) <= 0.0)
    {
        discard;
    }

    outColor = vec4((NDotL * ubo.lightColor.rgb + radiance) * subpassLoad(samplerAlbedo).rgb, 1.0);
}