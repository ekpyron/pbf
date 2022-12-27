#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fCoords;
layout(location = 1) flat in uint fAux;
layout(location = 2) in vec3 fGrid;
layout(location = 3) in vec3 fPosition;
layout(location = 4) in vec3 fVelocity;
layout(location = 5) flat in uint fType;


layout(binding = 0, std140) uniform GlobalUniformBuffer {
    mat4 mat;
    mat4 invviewmat;
    mat4 viewmat;
    mat3 viewRot;
} ubo;

void main() {
    float r = length(fCoords);
    if (r > 1.0) {
        discard;
    }

    vec3 normal = normalize(vec3(fCoords, -sqrt(1 - r)));

    vec4 fPos = vec4(fPosition - 0.1 * normal, 1.0);
    /*vec4 clipPos = projmat * fPos;
    fload d = clipPos.z / clipPos.w;
    gl_FragDepth = d;*/

//    vec3 lightdir = vec3(32,32,32) - (ubo.invviewmat * fPos).xyz;
    vec3 lightdir = vec3(ubo.invviewmat *  vec4(normalize(vec3(1,1,1)),0)).xyz;
    float lightdist = length(lightdir);
    lightdir /= lightdist;

    float intensity = max(dot(lightdir, normal), 0);
//    intensity /= lightdist * lightdist;
    intensity *= 1.0;

//    float angle = dot(normalize(vec3(-0.25,1,-0.25)), -lightdir);
//    intensity *= pow(angle, 2.0);

    intensity += 0.25;

    outColor = vec4(/* (1.0 - 0.25*length(fCoords)) * */ intensity * vec3(0.1,0.25,1), 1);
    if (fAux == -1u)
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    else if (fAux != 0)
        outColor = vec4(1.0, 1.0, 0.0, 1.0);

#if 0
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
#endif

    outColor = vec4(abs(fVelocity) / 64.0, 1.0);

    const vec4[2] colors = {
        vec4(1,0,0,1),
        vec4(0,1,0,1),
    };
    outColor = colors[fType];
}
