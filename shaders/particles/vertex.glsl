#version 430 core

// input vertex attributes
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 particlePosition;
layout (location = 2) in vec3 vColor;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 mvpmat;
};

// output to the fragment shader
out vec3 fNormal;
out vec3 fPosition;
out vec3 fColor;

void main (void)
{
	// pass data to the fragment shader
	fNormal = vPosition;
	vec3 pos = 0.2 * particlePosition + 0.2 * vec3 (-32, 1, -32) + vPosition * 0.1;
	fPosition = pos;
	fColor = vColor;
	// compute and output the vertex position
	// after view transformation and projection
	gl_Position = mvpmat * vec4 (pos, 1.0);
}
