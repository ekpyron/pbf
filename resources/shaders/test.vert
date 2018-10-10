#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 vertices[] = {
        vec4(0, -0.5, 0, 1),
        vec4(0.5, 0.5, 0, 1),
        vec4(-0.5, 0.5, 0, 1)
    };
    gl_Position = vertices[gl_VertexIndex];
}
