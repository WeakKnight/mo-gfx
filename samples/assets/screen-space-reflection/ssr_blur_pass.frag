#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerHdr;
layout (binding = 1) uniform sampler2D samplerSSR;

layout (location = 0) out vec4 outColor;

float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

void main()
{
    //declare stuff
    const int mSize = 5;
    const int kSize = (mSize-1)/2;
    float kernel[mSize];
    vec3 final_colour = vec3(0.0);
    
    //create the 1-D kernel
    float sigma = 3.0;
    float Z = 0.0;
    for (int j = 0; j <= kSize; ++j)
    {
        kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), sigma);
    }
    
    //get the normalization factor (as the gaussian has been clamped)
    for (int j = 0; j < mSize; ++j)
    {
        Z += kernel[j];
    }
    
    //read out the texels
    for (int i=-kSize; i <= kSize; ++i)
    {
        for (int j=-kSize; j <= kSize; ++j)
        {
            vec2 targetUV = inUV.xy + (vec2(float(i),float(j))) /vec2(800.0, 600.0);
            
            if(length(texture(samplerSSR, targetUV).rgb) <= 0.0001)
            {
                targetUV = inUV.xy;
            }

            final_colour += kernel[kSize+j]*kernel[kSize+i]*texture(samplerSSR, targetUV).rgb;
        }
    }
    
	final_colour = final_colour/(Z*Z);
  //   final_colour = texture(samplerSSR, inUV).rgb; 
    vec3 compositeColor = texture(samplerHdr, inUV).rgb + final_colour;

    outColor =  vec4(compositeColor, 1.0);
}