#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform GlobalUniformBuffer {
    mat4 mat;
} ubo;

layout(location = 0) out vec3 fColor;
layout(location = 0) in vec3 vPosition;

void main() {
    vec3 colors[] = {
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1),
        vec3(1, 1, 1)
    };
    gl_Position = ubo.mat * vec4(vPosition + vec3(gl_InstanceIndex, 0, 0), 1);
    fColor = colors[gl_VertexIndex]; // ubo.mat[gl_VertexIndex].xyz;
}
