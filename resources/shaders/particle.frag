#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fCoords;
layout(location = 1) flat in uint fAux;
layout(location = 2) in vec3 fGrid;

void main() {
    if (length(fCoords) > 1.0) {
        discard;
    }
    // outColor = vec4(fCoords * 0.5 + 0.5, 0, 1);

    outColor = vec4(fGrid, 1.0);
/*
    if (fAux == -1u)
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    */

        switch (fAux) {
            case 0:
            break;
            case 1:
            outColor = vec4(1,0,0, 1);
            break;
            case 2:
            outColor = vec4(0,1,0, 1);
            break;
            case 3:
            outColor = vec4(0,0,1, 1);
            break;
            case -1u:
            outColor = vec4(1,1,1, 1);
            break;
        }

    //outColor = vec4(color, 1.0);

    //outColor = vec4(float(fAux) / (64.0*64.0*64.0), 0, 0, 1);
}
