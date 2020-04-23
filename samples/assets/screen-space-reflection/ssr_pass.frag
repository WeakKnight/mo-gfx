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

layout(binding = 4) uniform samplerCube skybox;

layout (location = 0) out vec4 outColor;

const float PI = 3.1415927;

#define Scale vec3(.8, .8, .8)
#define K 19.19

vec3 Hash(vec3 a)
{
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}

void BranchlessONB(in vec3 n, inout vec3 b1, inout vec3 b2)
{
	float sign = sign(n.z);
	float a = -1.0 / (sign + n.z);
	float b = n.x * n.y * a;
	b1 = vec3(1.0 + sign * n.x * n.x * a, sign * b, -sign * n.x);
	b2 = vec3(b, sign + n.y * n.y * a, -n.y);
}

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

float Schlick(float NDotV)
{
    float r0 = 0.02;
    float reflectance = r0 + (1.0 - r0) * pow(1.0 - NDotV, 5.0);
    
    return reflectance;
}

void main()
{
    bool closeSSR = ubo.screenSize.z  > 0.5? true:false;  
    vec4 normalRoughness = texture(samplerNormalRoughness, inUV);
    float roughness = normalRoughness.w;

    mat4 viewInv = inverse(ubo.view);
    mat4 projInv = inverse(ubo.proj);

    float currentDepth = texture(samplerDepth, inUV).r;

    vec3 posCS = ScreenSpaceToViewSpace(inUV, projInv);
    vec3 posWorld = vec3(viewInv * vec4(posCS, 1.0));

    vec3 N = normalize(normalRoughness.xyz * 2.0 - vec3(1.0));
    vec3 V =  normalize(-posCS);
    vec3 L = normalize(mat3(ubo.view) * -ubo.lightDir.xyz);
    vec3 R = normalize(Hash(posWorld) * 0.02 + normalize(reflect(-V, N))); 
    
    // Discard Skybox
    if(length(normalRoughness.xyz) <= 0.0)
    {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Discard Non Reflective Surface
    if(roughness > 0.5)
    {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float maxDistance = 15;
    float resolution  = 0.2;
    int   steps       = 5;
    float thickness   = 0.5;

    vec2 texSize   = ubo.screenSize.xy;
    vec2 texCoord = inUV;

    vec4 uv = vec4(0);

    vec3 startView  = posCS;
    vec2 startUV = ViewSpaceToScreenSpace(startView);
    vec2 startFrag = startUV * texSize;

    vec3 endView    = startView + maxDistance * R;
    vec2 endUV = ViewSpaceToScreenSpace(endView);
    vec2 endFrag = endUV * texSize;
  
    vec2 frag  = startFrag.xy;
    uv.xy = frag / texSize;

    float deltaX    = endFrag.x - startFrag.x;
    float deltaY    = endFrag.y - startFrag.y;
    float useX      = abs(deltaX) >= abs(deltaY) ? 1 : 0;
    float delta     = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0, 1);
    vec2  increment = vec2(deltaX, deltaY) / max(delta, 0.001);

    float search0 = 0;
    float search1 = 0;

    int hit0 = 0;
    int hit1 = 0;

    float viewDistance = startView.z;
    float depth        = thickness;

    float i = 0;

    vec3 positionFrom = posCS;
    vec3 positionTo = posCS;

    int maxLoop = 64;
    for (i = 0; i < int(delta); ++i) 
    {
        if(i > maxLoop)
        {
            break;
        }

        frag      += increment;
        uv.xy      = frag / texSize;
        positionTo = ScreenSpaceToViewSpace(uv.xy, projInv);

        search1 =
        mix
            ( (frag.y - startFrag.y) / deltaY
            , (frag.x - startFrag.x) / deltaX
            , useX
            );

        search1 = clamp(search1, 0, 1);

        viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
        depth        = viewDistance - positionTo.z;

        if (depth < 0 && abs(depth) < thickness) 
        {
            hit0 = 1;
            break;
        } 
        else 
        {
            search0 = search1;
        }
    }

    search1 = search0 + ((search1 - search0) / 2);

    steps *= hit0;

    for (i = 0; i < steps; ++i) 
    {
        frag       = mix(startFrag.xy, endFrag.xy, search1);
        uv.xy      = frag / texSize;
        positionTo = ScreenSpaceToViewSpace(uv.xy, projInv);

        viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
        depth        = viewDistance - positionTo.z;

        if (depth < 0 && abs(depth) < thickness) 
        {
            hit1 = 1;
            search1 = search0 + ((search1 - search0) / 2);
        } else 
        {
            float temp = search1;
            search1 = search1 + ((search1 - search0) / 2);
            search0 = temp;
        }
    }

    float visibility =
      hit1
    * ( 1
      - max
         ( dot(V, R)
         , 0
         )
      )
    * ( 1
      - clamp
          ( depth / thickness
          , 0
          , 1
          )
      )
    * ( 1
      - clamp
          (   length(positionTo - positionFrom)
            / maxDistance
          , 0
          , 1
          )
      )
    * (uv.x < 0 || uv.x > 1 ? 0 : 1)
    * (uv.y < 0 || uv.y > 1 ? 0 : 1);
    
    float reflectance = Schlick(dot(N,V));
    vec3 originalColor = vec3(0.73, 0.95, 0.78);

    vec3 finalColor = (1 - reflectance) * originalColor + reflectance * ((1-visibility) * texture(skybox, mat3(viewInv) * R).rgb + visibility * texture(samplerHDR, uv.xy).rgb);
    if(closeSSR)
    {
        outColor = vec4(originalColor, 1.0);
    }
    else
    {
        outColor = vec4(finalColor, 1.0);
    }
}