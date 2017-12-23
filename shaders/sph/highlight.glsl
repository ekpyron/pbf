layout (local_size_x = BLOCKSIZE) in;

struct ParticleKey {
	vec3 pos;
	int id;
};

layout (std430, binding = 1) readonly buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

layout (binding = 2) uniform isamplerBuffer neighbourcelltexture;
layout (binding = 0, r32ui) uniform uimageBuffer highlighttexture;

void main (void)
{
	int id = particlekeys[gl_GlobalInvocationID.x].id;

	uint flag = imageLoad (highlighttexture, id).x;
	
	if ((flag & 1) == 1)
	{
		FOR_EACH_NEIGHBOUR(j)
		{
			imageAtomicOr (highlighttexture, particlekeys[j].id, uint(2));
		}
		END_FOR_EACH_NEIGHBOUR(j)
	}
}
