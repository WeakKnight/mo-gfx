#version 450

layout (location = 0) in vec3 inPos;

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
} ubo;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW = inPos;
	outUVW.x *= -1.0;
	// outUVW.y *= -1.0;
	vec4 pos = ubo.proj * mat4(mat3(ubo.view)) * vec4(inPos.xyz, 1.0);
	gl_Position = pos.xyww;
}
