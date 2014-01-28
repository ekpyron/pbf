#version 430 core

// input vertex attributes
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 particlePosition;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 mvpmat;
	mat4 invpmat;
};

void main (void)
{
	// compute and output the vertex position
	// after view transformation and projection
	vec3 pos = 0.2 * particlePosition + 0.2 * vec3 (-32, 1, -32) + vPosition * 0.1;
	gl_Position = mvpmat * vec4 (pos, 1.0);
}
