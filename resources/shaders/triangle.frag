#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fCoords;

void main() {
    if (length(fCoords) > 1.0) {
        discard;
    }
    outColor = vec4(fCoords * 0.5 + 0.5, 0, 1);
}
