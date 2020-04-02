#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler2D texSampler;

void main() {
    outColor = vec4(fragColor * textureLod(texSampler, fragTexCoord, 5.0).rgb, 1.0);
}