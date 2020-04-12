#version 450

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
    // split 0, split 1, split2
    vec4 splitPoints;
    vec4 nothing;
	vec4 nothing1;
	vec4 nothing2;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{    
    vec4 pos  = ubo.proj * ubo.view * vec4(inPosition, 1.0);
    gl_Position = pos;
}