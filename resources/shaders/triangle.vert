#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 fColor;

void main() {
    vec2 vertices[] = {
        vec2(0, -0.5),
        vec2(0.5, 0.5),
        vec2(-0.5, 0.5)
    };
    vec3 colors[] = {
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1)
    };
    gl_Position = vec4(vertices[gl_VertexIndex], 0, 1);
    fColor = colors[gl_VertexIndex];
}
