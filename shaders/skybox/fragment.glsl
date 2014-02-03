#version 430 core

// output color
layout (location = 0) out vec4 color;

// texture
layout (binding = 0) uniform samplerCube tex;

// input from vertex shader
in vec3 fTexcoord;

void main (void)
{
	// fetch texture value and output resulting color
	color = texture (tex, fTexcoord);
}
