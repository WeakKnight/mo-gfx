#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerSSRBlur;

layout (binding = 1) uniform PresentUniformBufferObject
{
    vec4 WidthHeightExposureNo;
    vec4 Nothing0;
    vec4 Nothing1;
    vec4 Nothing2;
} ubo;

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


vec3 Uncharted2Tonemap(vec3 x)
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 ScreenSpaceDither( vec2 vScreenPos )
{
	// Iestyn's RGB dither (7 asm instructions) from Portal 2 X360, slightly modified for VR
	//vec3 vDither = vec3( dot( vec2( 171.0, 231.0 ), vScreenPos.xy + iTime ) );
    vec3 vDither = vec3( dot( vec2( 171.0, 231.0 ), vScreenPos.xy ) );
    vDither.rgb = fract( vDither.rgb / vec3( 103.0, 71.0, 97.0 ) );
    
    //note: apply triangular pdf
    //vDither.r = remap_noise_tri_erp(vDither.r)*2.0-0.5;
    //vDither.g = remap_noise_tri_erp(vDither.g)*2.0-0.5;
    //vDither.b = remap_noise_tri_erp(vDither.b)*2.0-0.5;
    
    return vDither.rgb / 255.0; //note: looks better without 0.375...

    //note: not sure why the 0.5-offset is there...
    //vDither.rgb = fract( vDither.rgb / vec3( 103.0, 71.0, 97.0 ) ) - vec3( 0.5, 0.5, 0.5 );
	//return (vDither.rgb / 255.0) * 0.375;
}

void main()
{
    const float exposure = 1.0;

    // vec3 hdrColor = subpassLoad(samplerHdr).rgb;
    vec3 hdrColor = FXAA();
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

    outColor = vec4(mappedColor + ScreenSpaceDither(inUV * vec2(ubo.WidthHeightExposureNo.x, ubo.WidthHeightExposureNo.y)), 1.0);
}