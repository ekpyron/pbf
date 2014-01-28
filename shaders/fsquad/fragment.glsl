#version 430 core

// enable early z culling
layout (early_fragment_tests) in;

// output color
layout (location = 0) out vec4 color;

// input from vertex shader
in vec2 fTexcoord;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 mvpmat;
	mat4 invpmat;
};

layout (binding = 0) uniform sampler2D depthtex;

void main (void)
{
	// fetch depth from texture
	float depth = texture (depthtex, fTexcoord).x;
	if (depth == 1.0)
		discard;
	// calculate normalized device coordinates
	vec3 p = 2 * vec3 (fTexcoord, depth) - 1;
	// invert projection
	vec4 pos = invpmat * vec4 (p, 1);
	pos.xyzw /= pos.w;
	// output position as color
	color = 0.1 * pos;
}
