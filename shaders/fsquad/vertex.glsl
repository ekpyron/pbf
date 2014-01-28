#version 430 core

// input vertex attributes
layout (location = 0) in vec2 vPosition;

// output to the fragment shader
out vec2 fTexcoord;

void main (void)
{
	// pass data to the fragment shader
	fTexcoord = 0.5 * vPosition + 0.5;
	// output the vertex position
	gl_Position = vec4 (vPosition, 0.0, 1.0);
}
