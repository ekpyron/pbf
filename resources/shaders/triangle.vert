#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform GlobalUniformBuffer {
    mat4 mat;
    mat3 viewRot;
} ubo;

layout(push_constant) uniform PushConstants {
    uint sourceIndex;
} pushConstants;

struct ParticleData {
    vec3 position;
};

layout(std430, binding = 1) buffer particleData
{
    ParticleData data[];
};

layout(location = 0) out vec2 fCoords;
layout(location = 0) in vec2 vPosition;

void main() {
    uint index = pushConstants.sourceIndex;
    gl_Position = ubo.mat * vec4(0.2 * ubo.viewRot * vec3(vPosition, 0.0) + data[index + gl_InstanceIndex].position, 1);
    fCoords = vPosition;
}
