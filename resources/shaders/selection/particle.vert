#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform GlobalUniformBuffer {
    mat4 mat;
    mat3 viewRot;
} ubo;

layout(location = 0) out uint fParticleID;
layout(location = 1) out vec2 fCoords;
layout(location = 0) in vec3 vParticlePosition;

const vec3 vPositions[] = {
    vec3(-1.0f, -1.0f, 0.0f),
    vec3(1.0f, -1.0f, 0.0f),
    vec3(1.0f, 1.0f, 0.0f),
    vec3(-1.0f, 1.0f, 0.0f)
};

void main() {
    gl_Position = ubo.mat * vec4(0.2 * ubo.viewRot * vPositions[gl_VertexIndex] + vParticlePosition, 1);
    fParticleID = gl_InstanceIndex;
}
