#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 posCameraSpace;
layout(location = 2) in vec3 normalCameraSpace;

layout(location = 0) out vec4 albedoTarget;
layout(location = 1) out vec4 normalRoughnessTarget;
layout(location = 2) out vec3 positionTarget;

layout(binding = 1) uniform sampler2D texSampler;

void main() 
{
    vec3 N = normalize(normalCameraSpace);
    vec3 posCS = posCameraSpace;
    
    albedoTarget = vec4(texture(texSampler, fragTexCoord).rgb, 1.0);
    
    // Map to [0, 1]
    vec3 mappedN = N / 2.0 + 0.5;
    normalRoughnessTarget = vec4(mappedN, 1.0);

    positionTarget = posCameraSpace;
}