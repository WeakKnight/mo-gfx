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

vec2 ViewSpaceToScreenSpace(vec3 viewPos)
{
    vec4 clipSpacePos = ubo.proj * vec4(viewPos, 1.0);
    vec3 ndcSpacePos = clipSpacePos.xyz / clipSpacePos.w;
    vec2 screenSpacePos = (ndcSpacePos.xy + vec2(1.0)) / 2.0;
    return screenSpacePos.xy;
}

vec3 ScreenSpaceToViewSpace(vec2 uv, mat4 projInv)
{
    float z = texture(samplerDepth, uv).r;
     
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = projInv * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}

void main()
{
    vec4 normalRoughness = texture(samplerNormalRoughness, inUV);
    float roughness = normalRoughness.w;

    mat4 viewInv = inverse(ubo.view);
    mat4 projInv = inverse(ubo.proj);

    float currentDepth = texture(samplerDepth, inUV).r;

    vec3 posCS = ScreenSpaceToViewSpace(inUV, projInv);

    vec3 N = normalize(normalRoughness.xyz * 2.0 - vec3(1.0));
    vec3 V =  normalize(-posCS);
    vec3 L = normalize(mat3(ubo.view) * -ubo.lightDir.xyz);
    vec3 R = normalize(reflect(-V, N));
    vec3 H = normalize((normalize(reflect(-L, N)) + R) * 0.5);
    
    // Discard Skybox
    if(length(normalRoughness.xyz) <= 0.0)
    {
        discard;
        return;
    }


    // Discard Non Reflective Surface
    if(roughness > 0.5)
    {
        discard;
        return;
    }

    const float CastDistance = 12.0;
    const float Resolution = 8.0;

    vec2 screenSize = ubo.screenSize.xy;

    vec3 startPos = posCS;
    vec2 startUV = ViewSpaceToScreenSpace(startPos);
    vec2 startFrag = startUV * screenSize;

    vec3 endPos = posCS + CastDistance * R;
    vec2 endUV = ViewSpaceToScreenSpace(endPos);
    vec2 endFrag = endUV * screenSize;

    vec2 deltaFrag = endFrag - startFrag;
    bool useX = false;
    if(abs(deltaFrag.x) > abs(deltaFrag.y))
    {
        useX = true;
    }

    vec2 delta = vec2(0.0);
    int corseLoopTime = 0;
    if(useX)
    {
        delta = deltaFrag / deltaFrag.x;
        corseLoopTime = int(deltaFrag.x / Resolution);
    }
    else
    {
        delta = deltaFrag / deltaFrag.y;
        corseLoopTime = int(deltaFrag.y / Resolution);
    }

    vec2 currentFrag = startFrag;
    // corse cast
    for(int i = 0; i < corseLoopTime; i++)
    {
        currentFrag = currentFrag + delta;
    }

    // refinement

     outColor = vec4(vec3(0.3, 0.2, 0.15) * texture(samplerHDR, inUV).rgb, 1.0);

}