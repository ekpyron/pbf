// header is included here

// enable early z culling
layout (early_fragment_tests) in;

// input from vertex shader
flat in int particleid;
in vec2 fTexcoord;

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

	if (particles[particleid].highlighted > 0)
	{
		particles[particleid].highlighted = 0;
		particles[particleid].color = vec3 (0.25, 0, 1);
	}
	else
	{
		particles[particleid].highlighted = 1;
		particles[particleid].color = vec3 (1, 0, 0);
	}
}
