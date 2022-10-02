#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out uint outParticleID;

layout(location = 0) in flat uint fParticleID;
layout(location = 1) in vec2 fCoords;

void main() {
    if (length(fCoords) > 1.0) {
        discard;
    }
    outParticleID = fParticleID;
}
