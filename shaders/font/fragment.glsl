#version 430 core

// output color
layout (location = 0) out vec4 color;

// font texture
layout (binding = 0) uniform sampler2D tex;

// texture coordinates
in vec2 fTexcoord;

void main (void)
{
	// output the color value
	color = texture (tex, fTexcoord).x * vec4 (1, 1, 1, 1);
}
