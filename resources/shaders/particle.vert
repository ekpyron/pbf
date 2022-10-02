#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform GlobalUniformBuffer {
    mat4 mat;
    mat3 viewRot;
} ubo;

layout(location = 0) out vec2 fCoords;
layout(location = 1) flat out uint fAux;
layout(location = 2) out vec3 fGrid;

layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec3 vParticlePosition;
layout(location = 2) in uint vAux;


void main() {
    gl_Position = ubo.mat * vec4(0.2 * ubo.viewRot * vec3(vPosition, 0.0) + vParticlePosition, 1);

//    fGrid = mod(vParticlePosition, vec3(4.0f,4.0f,4.0f)) / 4.0f;
    fGrid = vParticlePosition - floor(vParticlePosition);

    fCoords = vPosition;
    fAux = vAux;
}
