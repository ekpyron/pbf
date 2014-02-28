#version 430 core

// input from vertex shader
flat in int particleid;
in vec2 fTexcoord;
in vec3 fPosition;
layout (location = 0) out int outputid;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 viewmat;
	mat4 projmat;
};

void main (void)
{
	float r = dot (fTexcoord, fTexcoord);
	if (r > 1.0)
	{
		discard;
		return;
	}
		
	vec3 normal = vec3 (fTexcoord, -sqrt (1 - r));
	
	vec4 fPos = vec4 (fPosition - 0.1 * normal, 1.0);
	vec4 clipPos = projmat * fPos;
	gl_FragDepth = clipPos.z / clipPos.w;
	
	outputid = particleid;
}
