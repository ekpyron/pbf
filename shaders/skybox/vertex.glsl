#version 430 core

// input vertex attributes
layout (location = 0) in vec3 vPosition;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 viewmat;
	mat4 projmat;
};

// output to the fragment shader
out vec3 fTexcoord;

void main (void)
{
	// pass data to the fragment shader
	fTexcoord = vPosition;
	// compute and output the vertex position
	// after view transformation and projection
	gl_Position = projmat * vec4 (100 * mat3 (viewmat) * vPosition, 1);
}
