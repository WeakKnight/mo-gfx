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
layout(binding = 6) uniform sampler2D shadowMap;

layout(binding = 0, set = 1) uniform ShadowUniformBufferObject 
{
    mat4 view;
    mat4 proj;
    vec4 nearFarSettings;
    vec4 nothing;
	vec4 nothing1;
	vec4 nothing2;
} subo; 

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

float ShadowedFactor(vec3 worldPos, float NDotL)
{
    vec2 shadowUV = WorldSpaceToScreenSpace(worldPos, subo.proj, subo.view);
    float shadowDepth = texture(shadowMap, shadowUV).r; 
    float fragDepth = WorldPosToDepth(worldPos, subo.proj, subo.view);
   
    shadowDepth = LinearizeDepth(shadowDepth, subo.nearFarSettings.x ,  subo.nearFarSettings.y);
    fragDepth = LinearizeDepth(fragDepth, subo.nearFarSettings.x , subo.nearFarSettings.y);

    if(fragDepth - 0.0001 < shadowDepth)
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

    if(length(normalRoughness.xyz) <= 0.0)
    {
        discard;
    }

    mat4 viewInv = inverse(ubo.view);
    mat4 projInv = inverse(ubo.proj);
    float currentDepth = texture(samplerDepth, inUV).r;

    vec3 posWS = WorldPosFromDepth(currentDepth, projInv, viewInv);
    vec3 posCS = vec3(ubo.view * vec4(posWS, 1.0));

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

    float shadowFactor = ShadowedFactor(posWS, NDotL);

    // outColor = vec4(shadowFactor * albedo + ambient + specular, 1.0);
    if(posCS.z > -30.0)
    {
        outColor = vec4(vec3(0.8,0.3,0.3) * albedo + 0.1 * ambient + specular, 1.0);
    }
    else
    {
        outColor = vec4(albedo + 0.1 * ambient + specular, 1.0);
    }
    // outColor = vec4( texture(shadowMap, inUV).rrr, 1.0);
    // outColor = vec4(inUV, 0.0, 1.0);
    // outColor = vec4(WorldSpaceToScreenSpace(posWS, subo.proj, subo.view), 0.0, 1.0);
}