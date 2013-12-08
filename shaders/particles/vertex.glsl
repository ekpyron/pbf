#version 430 core

// input vertex attributes
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 particlePosition;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 mvpmat;
};

// output to the fragment shader
out vec3 fNormal;
out vec3 fPosition;

void main (void)
{
	// pass data to the fragment shader
	fNormal = vPosition;
	vec3 pos = 0.2 * particlePosition + 0.2 * vec3 (-128, 1, -128) + vPosition * 0.1;
	fPosition = pos;
	// compute and output the vertex position
	// after view transformation and projection
	gl_Position = mvpmat * vec4 (pos, 1.0);
}
