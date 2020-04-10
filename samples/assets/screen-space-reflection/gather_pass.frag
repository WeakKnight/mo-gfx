#version 450

layout (location = 0) in vec2 inUV;

layout (input_attachment_index = 0, binding = 0) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput samplerNormalRoughness;

layout(binding = 2) uniform UniformBufferObject 
{
    vec4 lightDir;
    vec4 lightColor;
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 3) uniform samplerCube skybox;
layout(binding = 4) uniform samplerCube irradianceMap;
layout(binding = 5) uniform sampler2D samplerDepth;

layout (location = 0) out vec4 outColor;

const float PI = 3.1415927;

vec3 WorldPosFromDepth(float depth, mat4 projInv, mat4 viewInv) {
    float z = depth;

    vec4 clipSpacePosition = vec4(inUV * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = projInv * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = viewInv * viewSpacePosition;

    return worldSpacePosition.xyz;
}

void main()
{    
    vec4 normalRoughness = subpassLoad(samplerNormalRoughness);
    vec3 N = normalize(normalRoughness.xyz * 2.0 - vec3(1.0));
    vec3 L = mat3(ubo.view) * -ubo.lightDir.xyz;
    mat4 viewInv = inverse(ubo.view);

    vec3 NWorld = normalize(mat3(viewInv) * N);
    vec3 radiance = texture(irradianceMap, NWorld).rgb;

    float NDotL = clamp(dot(N,L), 0.0, 1.0);

    float roughness = normalRoughness.w;

    if(length(normalRoughness.xyz) <= 0.0)
    {
        discard;
    }

    mat4 projInv = inverse(ubo.proj);
    vec3 posWS = WorldPosFromDepth(texture(samplerDepth, inUV).r, projInv, viewInv);
    vec3 posCS = vec3(ubo.view * vec4(posWS, 1.0));

    outColor = vec4((NDotL * ubo.lightColor.rgb / PI + radiance) * subpassLoad(samplerAlbedo).rgb, 1.0);
    // outColor = vec4(posCS, 1.0);
}