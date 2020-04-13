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
layout(binding = 6) uniform sampler2D shadowMap0;
layout(binding = 7) uniform sampler2D shadowMap1;
layout(binding = 8) uniform sampler2D shadowMap2;

layout(binding = 0, set = 1) uniform ShadowUniformBufferObject0 
{
    mat4 view;
    mat4 proj;
    // split 0, split 1, split2, currentIndex
    vec4 splitPoints;
    vec4 nothing;
	vec4 nothing1;
	vec4 nothing2;
} subo0; 

layout(binding = 0, set = 2) uniform ShadowUniformBufferObject1 
{
    mat4 view;
    mat4 proj;
    // split 0, split 1, split2, currentIndex
    vec4 splitPoints;
    vec4 nothing;
	vec4 nothing1;
	vec4 nothing2;
} subo1; 

layout(binding = 0, set = 3) uniform ShadowUniformBufferObject2 
{
    mat4 view;
    mat4 proj;
    // split 0, split 1, split2, currentIndex
    vec4 splitPoints;
    vec4 nothing;
	vec4 nothing1;
	vec4 nothing2;
} subo2; 

layout (location = 0) out vec4 outColor;

const float PI = 3.1415927;

float LinearizeDepth(float depth, float n, float f)
{
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));	
}

vec3 WorldPosFromDepth(float depth, mat4 projInv, mat4 viewInv) {
    float z = depth;

    vec4 clipSpacePosition = vec4(inUV * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = projInv * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = viewInv * viewSpacePosition;

    return worldSpacePosition.xyz;
}

float WorldPosToDepth(vec3 worldPos, mat4 proj, mat4 view)
{
    vec4 projCoords = proj * view * vec4(worldPos, 1.0);
    vec3 projectedCoords = projCoords.xyz / projCoords.w;
    return projectedCoords.z;
}

vec2 WorldSpaceToScreenSpace(vec3 worldPos, mat4 projMatrix, mat4 viewMatrix)
{
    vec4 clipSpacePos = projMatrix * viewMatrix * vec4(worldPos, 1.0);
    vec3 ndcSpacePos = clipSpacePos.xyz / clipSpacePos.w;
    vec2 screenSpacePos = (ndcSpacePos.xy + vec2(1.0)) / 2.0;
    return screenSpacePos.xy;
}

float ShadowedFactor(vec3 worldPos, float NDotL, uint usedCascade)
{
    float shadowDepth = 1.0;
    float fragDepth = 1.0;

    if(usedCascade == 0)
    {
        vec2 shadowUV = WorldSpaceToScreenSpace(worldPos, subo0.proj, subo0.view);
        shadowDepth = texture(shadowMap0, shadowUV).r; 
        fragDepth = WorldPosToDepth(worldPos, subo0.proj, subo0.view);
    }
    else if(usedCascade == 1)
    {
        vec2 shadowUV = WorldSpaceToScreenSpace(worldPos, subo1.proj, subo1.view);
        shadowDepth = texture(shadowMap1, shadowUV).r; 
        fragDepth = WorldPosToDepth(worldPos, subo1.proj, subo1.view);
    }
    else if(usedCascade == 2)
    {
        vec2 shadowUV = WorldSpaceToScreenSpace(worldPos, subo2.proj, subo2.view);
        shadowDepth = texture(shadowMap2, shadowUV).r; 
        fragDepth = WorldPosToDepth(worldPos, subo2.proj, subo2.view);
    }

    if(fragDepth - 0.0005 < shadowDepth)
    {
        return 1.0;
    }
    else
    {
        return 0.0;
    }
}

void main()
{       
    vec4 normalRoughness = subpassLoad(samplerNormalRoughness);

    // if(inUV.x < 0.5)
    // {
    //     outColor = vec4( texture(shadowMap0,  vec2((inUV.x) * 2, inUV.y)).rrr, 1.0);
    //     return;
    // }

    if(length(normalRoughness.xyz) <= 0.0)
    {
        discard;
    }

    mat4 viewInv = inverse(ubo.view);
    mat4 projInv = inverse(ubo.proj);
    float currentDepth = texture(samplerDepth, inUV).r;

    vec3 posWS = WorldPosFromDepth(currentDepth, projInv, viewInv);
    vec3 posCS = vec3(ubo.view * vec4(posWS, 1.0));

    uint usedCascade = 0;
    if(posCS.z < subo0.splitPoints.x)
    {
        usedCascade = 1;
    }
    if(posCS.z < subo0.splitPoints.y)
    {
        usedCascade = 2;
    }

    vec3 N = normalize(normalRoughness.xyz * 2.0 - vec3(1.0));
    vec3 V =  normalize(-posCS);
    vec3 L = normalize(mat3(ubo.view) * -ubo.lightDir.xyz);
    vec3 R = normalize(reflect(-L, N));
    vec3 H = normalize((V + R) * 0.5);

    vec3 RealR = normalize(reflect(-V, N));

    float NDotL = clamp(dot(N,L), 0.0, 1.0);
    float HDotN = clamp(dot(H, N), 0.0, 1.0);

    vec3 NWorld = normalize(mat3(viewInv) * N);
    vec3 radiance = texture(irradianceMap, vec3(-1.0, 1.0, 1.0) * NWorld).rgb;
    float roughness = normalRoughness.w;

    vec3 specular = vec3(0.0, 0.0, 0.0);
    vec3 albedo = (NDotL * ubo.lightColor.rgb) * subpassLoad(samplerAlbedo).rgb;
    vec3 ambient = radiance * subpassLoad(samplerAlbedo).rgb;
    
    if(roughness < 0.5)
    {
        specular = texture(skybox, vec3(-1.0, 1.0, 1.0) * normalize(mat3(viewInv) * RealR)).rgb;
        albedo = vec3(0.0, 0.0, 0.0);
    }

    float shadowFactor = ShadowedFactor(posWS, NDotL, usedCascade);
    vec3 blendColor = vec3(1.0, 1.0, 1.0);
    
    // if(usedCascade == 0)
    // {
    //     blendColor = vec3(0.8, 0.3, 0.3);
    // }
    // else if(usedCascade == 1)
    // {
    //     blendColor = vec3(0.3, 0.8, 0.3);
    // }
    // else if(usedCascade == 2)
    // {
    //     blendColor = vec3(0.3, 0.3, 0.8);
    // }

    outColor = vec4(blendColor * (shadowFactor * albedo + ambient + specular), 1.0);

    // if(posCS.z > -30.0)
    // {
    //     outColor = vec4(vec3(0.8,0.3,0.3) * shadowFactor * albedo + 0.1 * ambient + specular, 1.0);
    // }
    // else
    // {
    //     outColor = vec4(albedo + 0.1 * ambient + specular, 1.0);
    // }
    // outColor = vec4( texture(shadowMap, inUV).rrr, 1.0);
    // outColor = vec4(inUV, 0.0, 1.0);
    // outColor = vec4(WorldSpaceToScreenSpace(posWS, subo.proj, subo.view), 0.0, 1.0);
}