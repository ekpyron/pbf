layout (local_size_x = BLOCKSIZE) in;

struct ParticleKey {
	vec3 pos;
	int id;
};

layout (std430, binding = 1) readonly buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

layout (binding = 0, rgba32f) uniform imageBuffer positiontexture;
layout (binding = 1, rgba32f) uniform imageBuffer velocitytexture;

void main (void)
{
	ParticleKey key = particlekeys[gl_GlobalInvocationID.x];
	
	vec3 oldposition = imageLoad (positiontexture, key.id).xyz;

	// calculate velocity
	vec3 velocity = (key.pos - oldposition) / timestep;
	
	// update position and velocity
	imageStore (positiontexture, key.id, vec4 (key.pos, 0));
	imageStore (velocitytexture, key.id, vec4 (velocity, 0));
}
