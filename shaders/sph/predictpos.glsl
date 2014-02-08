// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

struct ParticleInfo
{
	vec3 position;
	bool highlighted;
	vec3 velocity;
	float density;
	vec3 color;
	float vorticity;	
};

struct ParticleKey
{
	vec3 position;
	uint id;
};

layout (std430, binding = 0) writeonly buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

layout (std430, binding = 1) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

uniform bool extforce;

void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.x;

	ParticleInfo particle = particles[particleid];
	
	vec3 velocity = particle.velocity;
	
	// optionally apply an additional external force to some particles
	if (extforce && particle.position.z > GRID_DEPTH/2)
		velocity += vec3 (0, 0, -20) * timestep;
	
	// gravity
	velocity += gravity * vec3 (0, -1, 0) * timestep;
	
	// predict new position
	particlekeys[particleid].position = particle.position + timestep * velocity;
	particlekeys[particleid].id = particleid;
	// set color
	particles[particleid].color = vec3 (0.25, 0, 1);
}
