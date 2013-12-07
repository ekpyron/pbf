#version 430 core

// vertex attributes
layout (location = 0) in vec2 vPosition;

// view/projection matrix
uniform mat4 mat;

// texture coord for the fragment shader
out vec2 fTexcoord;

// which character to display
uniform vec2 charpos;

// raster position for the character
uniform vec2 pos;

void main (void)
{
	// determine the texture coordinates for the character
	fTexcoord = charpos + vPosition / vec2 (16, 8);
	// output the position
	gl_Position = mat * vec4 (pos + vPosition, 0, 1);
}
