#version 430 core

// enable early z culling
layout (early_fragment_tests) in;

// input from vertex shader
flat in int particleid;
in vec2 fTexcoord;

struct ParticleInfo
{
	vec3 position;
	vec4 oldposition;
};

layout (std430, binding = 0) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

layout (std430, binding = 1) writeonly buffer AuxBuffer
{
	vec4 auxdata[];
};

uniform bool write;

void main (void)
{
	float r = dot (fTexcoord, fTexcoord);
	if (r > 1)
	{
		discard;
		return;
	}

	if (write && particleid != -1)
	{
		if (particles[particleid].oldposition.w == 0)
		{
			particles[particleid].oldposition.w = 1;
			auxdata[particleid] = vec4 (1, 0, 0, 1);
		}
		else
		{
			particles[particleid].oldposition.w = 0;
			auxdata[particleid] = vec4 (0.25, 0, 1, 1);
		}
	}
}
