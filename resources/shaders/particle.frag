#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fCoords;
layout(location = 1) in float fAux;

void main() {
    if (length(fCoords) > 1.0) {
        discard;
    }
    // outColor = vec4(fCoords * 0.5 + 0.5, 0, 1);
    outColor = vec4(fAux/128, 0.0, 0.0, 1.0);
}
