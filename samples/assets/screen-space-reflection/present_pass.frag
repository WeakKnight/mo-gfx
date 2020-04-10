#version 450

layout (location = 0) in vec2 inUV;

// layout (input_attachment_index = 0, binding = 0) uniform subpassInput samplerHdr;
layout (binding = 0) uniform sampler2D samplerHdr;

layout (location = 0) out vec4 outColor;

const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;
const vec3 W = vec3(11.2);

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
    vec3 hdrColor = texture(samplerHdr, inUV).rgb;

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

    outColor = vec4(mappedColor + ScreenSpaceDither(inUV * vec2(800.0, 600.0)), 1.0);
}