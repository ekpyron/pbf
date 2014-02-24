// header is included here

// input from vertex shader
flat in int particleid;
in vec2 fTexcoord;
layout (location = 0) out int outputid;

layout (std430, binding = 0) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

void main (void)
{
	float r = dot (fTexcoord, fTexcoord);
	if (r > 0.5)
	{
		discard;
		return;
	}
	
	outputid = particleid;
}
