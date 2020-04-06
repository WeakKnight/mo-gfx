#version 450

layout (location = 0) in vec2 inUV;

layout (input_attachment_index = 0, binding = 0) uniform subpassInput samplerHdr;

layout (location = 0) out vec4 outColor;

void main()
{
    const float exposure = 1.5;

    vec3 hdrColor = subpassLoad(samplerHdr).rgb;
    vec3 mappedColor = vec3(1.0) - exp(-hdrColor * exposure);
     // Gamma Correction
    mappedColor = pow(mappedColor, vec3(1.0 / 2.2));

    // Vignette 
    const float strength = 12.0;
    const float power = 0.13;
    vec2 tuv = inUV * (vec2(1.0) - inUV.yx);
    float vign = tuv.x*tuv.y * strength;
    vign = pow(vign, power);
    mappedColor *= vign;

    outColor = vec4(mappedColor, 1.0);
}