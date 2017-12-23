layout (local_size_x = BLOCKSIZE) in;

struct ParticleKey {
	vec3 pos;
	int id;
};

layout (std430, binding = 1) writeonly buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

layout (location = 0) uniform bool extforce;

layout (binding = 0) uniform samplerBuffer positiontexture;
layout (binding = 1) uniform samplerBuffer velocitytexture;

void main (void)
{
	ParticleKey key;
	key.id = int (gl_GlobalInvocationID.x);

	key.pos = texelFetch (positiontexture, key.id).xyz;
	vec3 velocity = texelFetch (velocitytexture, key.id).xyz;

	// optionally apply an additional external force to some particles
	if (extforce && key.pos.z > GRID_SIZE.z/2)
		velocity += 2 * gravity * vec3 (0, 0, -1) * timestep;
	
	// gravity
	//velocity += gravity * vec3 (n.x * 0.05, -1, n.y*0.05) * timestep;
	velocity += gravity * vec3 (0, -1, 0) * timestep;

	key.pos += timestep * velocity;

	// predict new position
	particlekeys[key.id] = key;
}
