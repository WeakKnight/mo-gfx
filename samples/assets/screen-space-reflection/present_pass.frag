#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerSSRBlur;

layout (binding = 1) uniform PresentUniformBufferObject
{
    vec4 WidthHeightExposureNo;
    vec4 Config0;
    vec4 Nothing1;
    vec4 Nothing2;
    mat4 view;
    mat4 proj;
} ubo;

layout (binding = 2) uniform sampler2D samplerDepth;

layout (location = 0) out vec4 outColor;

const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;
const vec3 W = vec3(11.2);

#define FXAA_SPAN_MAX 8.0
#define FXAA_REDUCE_MUL   (1.0/FXAA_SPAN_MAX)
#define FXAA_REDUCE_MIN   (1.0/128.0)
#define FXAA_SUBPIX_SHIFT (1.0/4.0)

vec3 FXAA() 
{
    vec2 rcpFrame = vec2(1.0)/textureSize(samplerSSRBlur, 0).xy;
    vec4 uv = vec4( inUV, inUV - (rcpFrame * (0.5 + FXAA_SUBPIX_SHIFT)));
    vec3 rgbNW = textureLod(samplerSSRBlur, uv.zw, 0.0).xyz;
    vec3 rgbNE = textureLod(samplerSSRBlur, uv.zw + vec2(1,0)*rcpFrame.xy, 0.0).xyz;
    vec3 rgbSW = textureLod(samplerSSRBlur, uv.zw + vec2(0,1)*rcpFrame.xy, 0.0).xyz;
    vec3 rgbSE = textureLod(samplerSSRBlur, uv.zw + vec2(1,1)*rcpFrame.xy, 0.0).xyz;
    vec3 rgbM  = textureLod(samplerSSRBlur, uv.xy, 0.0).xyz;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) * rcpFrame.xy;

    vec3 rgbA = (1.0/2.0) * (
        textureLod(samplerSSRBlur, uv.xy + dir * (1.0/3.0 - 0.5), 0.0).xyz +
        textureLod(samplerSSRBlur, uv.xy + dir * (2.0/3.0 - 0.5), 0.0).xyz);
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        textureLod(samplerSSRBlur, uv.xy + dir * (0.0/3.0 - 0.5), 0.0).xyz +
        textureLod(samplerSSRBlur, uv.xy + dir * (3.0/3.0 - 0.5), 0.0).xyz);
    
    float lumaB = dot(rgbB, luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax)) return rgbA;
    
    return rgbB; 
}

vec3 Dilation(sampler2D colorTexture)
{
    int   size         = 6;
    float separation   = 1.0;
    float minThreshold = 0.2;
    float maxThreshold = 0.6;

    vec2 texSize   = textureSize(colorTexture, 0).xy;
    vec2 fragCoord = inUV * texSize;

    vec4 fragColor = texture(colorTexture, fragCoord / texSize);

    float  mx = 0.0;
    vec4  cmx = fragColor;

    for (int i = -size; i <= size; ++i) {
        for (int j = -size; j <= size; ++j) {
        // For a rectangular shape.
        //if (false);

        // For a diamond shape;
        // if (!(abs(i) <= size - abs(j))) { continue; }

        // For a circular shape.
        if (!(distance(vec2(i, j), vec2(0, 0)) <= size)) { continue; }
        vec2 targetUV =  ( fragCoord
                + (vec2(i, j) * separation)
                )
                / texSize;

        if(texture(samplerDepth, targetUV).r > 0.9999)
        {
            continue;
        }

        vec4 c =
            texture
            ( colorTexture
            ,  targetUV
            );

        float mxt = dot(c.rgb, vec3(0.21, 0.72, 0.07));

        if (mxt > mx) {
            mx = mxt;
            cmx = c;
        }
        }
    }

    fragColor.rgb =
        mix
        ( fragColor.rgb
        , cmx.rgb
        , smoothstep(minThreshold, maxThreshold, mx)
        );

    return fragColor.rgb;
}

vec3 ScreenSpaceToViewSpace(vec2 uv, mat4 projInv)
{
    float z = texture(samplerDepth, uv).r;
     
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = projInv * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}

vec3 Uncharted2Tonemap(vec3 x)
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 ScreenSpaceDither( vec2 vScreenPos )
{
    vec3 vDither = vec3( dot( vec2( 171.0, 231.0 ), vScreenPos.xy ) );
    vDither.rgb = fract( vDither.rgb / vec3( 103.0, 71.0, 97.0 ) );
    
    return vDither.rgb / 255.0;
}

void main()
{
    bool visCloseDof = ubo.Config0.x > 0.5?true:false; 
    bool visCloseDithering = ubo.Config0.y > 0.5?true:false; 

    const float exposure = 1.0;
    float minDistance = 14.0;
    float maxDistance = 25.0;

    mat4 projInv = inverse(ubo.proj);
    vec3 posCS = ScreenSpaceToViewSpace(inUV, projInv);

    // vec3 hdrColor = subpassLoad(samplerHdr).rgb;
    vec3 hdrColor = FXAA();

    if(!visCloseDof)
    {
        float depth = texture(samplerDepth, inUV).r;

        if(depth <= 0.9999)
        {
            vec3 outOfFocusColor = Dilation(samplerSSRBlur);
            float dofOffset = 0.01;
            vec3 focusPoint = vec3(0.0, 0.0, 15.0);

            float blur =
            smoothstep
            ( minDistance
            , maxDistance
            , abs(abs(posCS.z) - focusPoint.z)
            );

            hdrColor = mix(hdrColor, outOfFocusColor, blur);
        }
    }

    // vec3 hdrColor = texture(samplerSSRBlur, inUV).rgb;

    // vec3 mappedColor = vec3(1.0) - exp(-hdrColor * exposure);
    //  // Gamma Correction
    // mappedColor = pow(mappedColor, vec3(1.0 / 2.2));

    float ExposureBias = 1.5;
    vec3 curr = Uncharted2Tonemap(ExposureBias*hdrColor);
    vec3 whiteScale = 1.0/Uncharted2Tonemap(W);
    vec3 mappedColor = curr*whiteScale;
    // mappedColor = pow(mappedColor, vec3(1/2.2));

    // Vignette 
    const float strength = 12.0;
    const float power = 0.13;
    vec2 tuv = inUV * (vec2(1.0) - inUV.yx);
    float vign = tuv.x*tuv.y * strength;
    vign = pow(vign, power);
    mappedColor *= vign;

    vec3 ditherColor = vec3(0.0);
    if(!visCloseDithering)
    {
        ditherColor = ScreenSpaceDither(inUV * vec2(ubo.WidthHeightExposureNo.x, ubo.WidthHeightExposureNo.y));
    }
    outColor = vec4(mappedColor + ditherColor, 1.0);
}