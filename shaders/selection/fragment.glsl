#version 430 core

// enable early z culling
layout (early_fragment_tests) in;

// input from vertex shader
flat in int particleid;
in vec2 fTexcoord;

struct ParticleInfo
{
	vec3 position;
	bool highlighted;
	vec3 velocity;
	float density;
	vec3 color;
	float vorticity;	
};

layout (std430, binding = 0) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

void main (void)
{
	float r = dot (fTexcoord, fTexcoord);
	if (r > 0.6)
	{
		discard;
		return;
	}

	if (particles[particleid].highlighted)
	{
		particles[particleid].highlighted = false;
		particles[particleid].color = vec3 (0.25, 0, 1);
	}
	else
	{
		particles[particleid].highlighted = true;
		particles[particleid].color = vec3 (1, 0, 0);
	}
}
