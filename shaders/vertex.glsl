#version 430 core

// input vertex attributes
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vTexcoord;
layout (location = 2) in vec3 vNormal;

// projection and view matrix
uniform mat4 projmat;
uniform mat4 viewmat;

// output to the fragment shader
out vec2 fTexcoord;
out vec3 fNormal;
out vec3 fPosition;

void main (void)
{
	// pass data to the fragment shader
	fTexcoord = vTexcoord;
	fNormal = vNormal;
	fPosition = vPosition;
	// compute and output the vertex position
	// after view transformation and projection
	gl_Position = projmat * viewmat * vec4 (vPosition, 1.0);
}
