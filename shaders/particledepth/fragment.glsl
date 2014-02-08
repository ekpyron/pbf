#version 430 core

// input from vertex shader
in vec3 fPosition;
in vec2 fTexcoord;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 viewmat;
	mat4 projmat;
};

void main (void)
{
	float r = dot (fTexcoord, fTexcoord);
	if (r > 1)
		discard;
		
	vec3 normal = vec3 (fTexcoord, -sqrt (1 - r));
	
	vec4 fPos = vec4 (fPosition - 0.1 * normal, 1.0);
	vec4 clipPos = projmat * fPos;
	
	// non-linear depth buffer
	gl_FragDepth = clipPos.z / clipPos.w;
}
