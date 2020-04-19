#version 450

layout (location = 0) in vec2 inUV;

layout (input_attachment_index = 0, binding = 0) uniform subpassInput samplerAlbedo;
layout (binding = 1) uniform sampler2D samplerNormalRoughness;

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
const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex)
{
	float shadow = 1.0;
	float bias = 0.0003;

	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) 
    {
        float dist = 0.0;
        if(cascadeIndex == 0)
        {
            dist = texture(shadowMap0, (shadowCoord.st + offset)).r;
        }
        else if(cascadeIndex == 1)
        {
            dist = texture(shadowMap1, (shadowCoord.st + offset)).r;
        }
        else if(cascadeIndex == 2)
        {
            dist = texture(shadowMap2, (shadowCoord.st + offset)).r;
        }

        if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) 
        {
            shadow = 0.0;
        }
	}

	return shadow;
}

float filterPCF(vec4 sc, uint cascadeIndex)
{
	float scale = 0.75;
	float dx = scale * 1.0 / 4096.0;
	float dy = scale * 1.0 / 4096.0;

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y), cascadeIndex);
			count++;
		}
	}
	return shadowFactor / count;
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

vec3 ViewSpacePositionFromUV(vec2 uv, mat4 viewInv, mat4 projInv)
{
    float z = texture(samplerDepth, uv).r;
     
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = projInv * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}


/*
pos -- view space,
dir -- view space,
viewInv -- inversed view matrix,
projInv -- inversed proj matrix
*/
float ScreenSpaceShadow(vec3 pos, vec3 dir, vec3 normal , mat4 viewInv, mat4 projInv)
{
    vec2 texSize = textureSize(samplerNormalRoughness, 0).xy;
    
    float maxDistance = 0.5;

    vec4 startView = vec4(pos.xyz + (dir * 0.0), 1);
    vec4 endView   = vec4(pos.xyz + (dir * maxDistance), 1);

    vec4 startFrag = startView;
    startFrag      = ubo.proj * startFrag;
    startFrag.xyz /= startFrag.w;
    startFrag.xy   = startFrag.xy * 0.5 + 0.5;
    startFrag.xy  *= texSize;

    vec4 endFrag = endView;
    endFrag      = ubo.proj * endFrag;
    endFrag.xyz /= endFrag.w;
    endFrag.xy   = endFrag.xy * 0.5 + 0.5;
    endFrag.xy  *= texSize;

    vec2 uv = vec2(0.0);

    // current position in pixel
    vec2 frag  = startFrag.xy;
    // current position's uv in depth texture
    uv.xy = frag / texSize;

    float deltaX    = endFrag.x - startFrag.x;
    float deltaY    = endFrag.y - startFrag.y;

    float delta = deltaY;
    bool useX = false;

    if(dot(dir, normal) < 0)
    {
        return 1.0;
    }

    if(abs(deltaX) > abs(deltaY))
    {
        delta = deltaX;
        useX = true;
    }

    vec2  increment = vec2(deltaX, deltaY) / abs(delta);
    bool hit = false;

    int loopCount = int(abs(delta));
    if(loopCount > 24)
    {
        loopCount = 16;
    }

    for (int i = 0; i < loopCount; i++) 
    {
        // Update Screen Space Position
        frag += increment;
        // Update Correspond UV
        uv = frag / texSize;

        if(uv.x > 1.0 || uv.x < 0.0 || uv.y > 1.0 || uv.y < 0.0)
        {
            break;
        }

        vec3 viewSpacePosition = ViewSpacePositionFromUV(uv, viewInv, projInv);

        // current ray march point camera position
        float ratio = 0.0;
        
        if(useX)
        {
            ratio = (frag.x - startFrag.x) / deltaX;
        }
        else
        {
            ratio = (frag.y - startFrag.y) / deltaY;
        }

        float raymarchDepth = (startView.z * endView.z) / mix(endView.z, startView.z, abs(ratio));
        float raymarchX = (startView.x * endView.x) / mix(endView.x, startView.x, abs(ratio));
        float raymarchY = (startView.y * endView.y) / mix(endView.y, startView.y, abs(ratio));

        vec3 raymarchPos = vec3(raymarchX, raymarchY, raymarchDepth);
        
        if((raymarchDepth < viewSpacePosition.z) && ((viewSpacePosition.z - raymarchDepth) < 0.07))
        {
            hit = true;
            break;
        }
    }

    if(hit)
    {
        return 0.0;
    }
    else
    {
        return 1.0;
    }
}

void main()
{       
    vec2 texSize  = textureSize(samplerNormalRoughness, 0).xy;
    vec2 texCoord = inUV;

    vec4 normalRoughness = texture(samplerNormalRoughness, inUV);

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

    vec4 shadowCoord = vec4(0.0);
    if(usedCascade == 0)
    {
        shadowCoord = (biasMat * subo0.proj * subo0.view) * vec4(posWS, 1.0);	
    }
    else if(usedCascade == 1)
    {
        shadowCoord = (biasMat * subo1.proj * subo1.view) * vec4(posWS, 1.0);	
    }
    else if(usedCascade == 2)
    {
        shadowCoord = (biasMat * subo2.proj * subo2.view) * vec4(posWS, 1.0);	
    }

    float shadowFactor = filterPCF(shadowCoord / shadowCoord.w, usedCascade);

    float ssShadowFactor = ScreenSpaceShadow(posCS, L, N, viewInv, projInv);

    // if(ssShadowFactor < shadowFactor)
    // {
    //     shadowFactor = ssShadowFactor;
    // }

    vec3 blendColor = vec3(1.0, 1.0, 1.0);
    outColor = vec4(blendColor * (shadowFactor * albedo + ambient + specular), 1.0);
    // outColor = vec4(posCS, 1.0);
    if(ssShadowFactor < 1.0)
    {
        outColor = vec4(1.0, 0.2, 0.2, 1.0) * outColor;
    }

    // vec4 startView = vec4(posCS, 1);
    // vec4 startFrag = startView;
    // startFrag      = ubo.proj * startFrag;
    // startFrag.xyz /= startFrag.w;
    // startFrag.xy   = startFrag.xy * 0.5 + 0.5;

    // vec4 endtView = vec4(posCS + L* 2.0, 1);
    // vec4 endFrag = endtView;
    // endFrag      = ubo.proj * endFrag;
    // endFrag.xyz /= endFrag.w;
    // endFrag.xy   = endFrag.xy * 0.5 + 0.5;

    // if(abs((inUV.x / inUV.y) - abs((endFrag - startFrag).x / (endFrag - startFrag).y)) < 0.01)
    // {
    //     outColor = vec4(1.0, 0.0, 0.0, 1.0);
    // }
    // outColor = vec4(startFrag.xy, 0.0, 1.0) ;
    // outColor = vec4(inUV, 0.0, 1.0) ;
}