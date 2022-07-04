#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform GlobalUniformBuffer {
    mat4 mat;
    mat3 viewRot;
} ubo;

struct ParticleData {
    vec3 position;
};

layout(std430, binding = 1) buffer particleData
{
    ParticleData data[];
};

layout(location = 0) out vec2 fCoords;
layout(location = 0) in vec3 vPosition;

void main() {
    vec2 coords[] = {
        vec2(-1, -1),
        vec2(1, -1),
        vec2(1, 1),
        vec2(-1, 1)
    };
    gl_Position = ubo.mat * vec4(0.2 * ubo.viewRot * vPosition + data[gl_InstanceIndex].position, 1);
    fCoords = coords[gl_VertexIndex]; // ubo.mat[gl_VertexIndex].xyz;
}
