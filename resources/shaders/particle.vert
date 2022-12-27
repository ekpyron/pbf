#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0, std140) uniform GlobalUniformBuffer {
    mat4 mat;
    mat4 invviewmat;
    mat4 viewmat;
    mat3 viewRot;
} ubo;

layout(location = 0) out vec2 fCoords;
layout(location = 1) flat out uint fAux;
layout(location = 2) out vec3 fGrid;
layout(location = 3) out vec3 fPosition;
layout(location = 4) out vec3 fVelocity;
layout(location = 5) out uint fType;

layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec3 vParticlePosition;
layout(location = 2) in uint vAux;
layout(location = 3) in vec3 vVelocity;
layout(location = 4) in uint vType;


void main() {
    vec3 pos = 0.3 * ubo.viewRot * vec3(vPosition, 0.0) + vParticlePosition;
    gl_Position = ubo.mat * vec4(pos, 1);

//    fGrid = mod(vParticlePosition, vec3(4.0f,4.0f,4.0f)) / 4.0f;
    fGrid = vParticlePosition - floor(vParticlePosition);

    fCoords = vPosition;
    fAux = vAux;
    fPosition = (ubo.viewmat * vec4(pos, 1.0)).xyz;
    fVelocity = vVelocity;
    fType = vType;
}
