// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

layout (std430, binding = 0) buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

layout (std430, binding = 1) buffer ParticleBuffer
{
	ParticleInfo particles[];
};


layout (binding = 0) uniform isamplerBuffer neighbourcelltexture;

#define FOR_EACH_NEIGHBOUR(var) for (int o = 0; o < 3; o++) {\
		ivec3 datav = texelFetch (neighbourcelltexture, int (gl_GlobalInvocationID.x * 3 + o)).xyz;\
		for (int comp = 0; comp < 3; comp++) {\
		int data = datav[comp];\
		int entries = data >> 24;\
		data = data & 0xFFFFFF;\
		if (data == 0) continue;\
		for (int var = data; var < data + entries; var++) {\
		if (var != gl_GlobalInvocationID.x) {
#define END_FOR_EACH_NEIGHBOUR(var)	}}}}

void main (void)
{
	ParticleKey key = particlekeys[gl_GlobalInvocationID.x];
	uint particleid = key.id;
	vec3 position = key.position;

	if (particles[particleid].highlighted > 0)
	{
		particles[particleid].color = vec3 (1, 0, 0);
		FOR_EACH_NEIGHBOUR(j)
		{
			particles[particlekeys[j].id].color = vec3 (0, 1, 0);
		}
		END_FOR_EACH_NEIGHBOUR(j)
	}
}
