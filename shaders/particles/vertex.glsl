#version 430 core

// input vertex attributes
layout (location = 0) in vec2 vPosition;
layout (location = 1) in vec3 particlePosition;
layout (location = 2) in uint highlight;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 viewmat;
	mat4 projmat;
	mat4 invviewmat;
};

// output to the fragment shader
out vec3 fPosition;
out vec3 fColor;
out vec2 fTexcoord;

void main (void)
{
	// pass data to the fragment shader
	vec4 pos = viewmat * vec4 (0.2 * particlePosition + 0.2 * vec3 (-64, 1, -64), 1.0);
	pos.xy += vPosition * 0.1;
	fPosition = pos.xyz;
	fTexcoord = vPosition;
	if (highlight == 0)
		fColor = vec3 (0.25, 0, 1);
	else if (highlight == 1)
		fColor = vec3 (1, 0, 0);
	else
		fColor = vec3 (0, 1, 0);
	// compute and output the vertex position
	// after view transformation and projection
	pos = projmat * pos;
	gl_Position = pos;
}
