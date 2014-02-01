// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

struct ParticleInfo
{
	vec3 position;
	vec4 oldposition;
};

layout (std430, binding = 0) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

uniform bool extforce;

void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.x;

	ParticleInfo particle = particles[particleid];
	
	vec3 velocity = (particle.position - particle.oldposition.xyz) / timestep;
	
	// optionally apply an additional external force to some particles
	if (extforce && particle.position.z > GRID_DEPTH/2)
		velocity += vec3 (0, 0, -20) * timestep;
	
	// gravity
	velocity += gravity * vec3 (0, -1, 0) * timestep;
	
	// save old position and predict new position
	particle.oldposition.xyz = particle.position;
	particle.position += timestep * velocity;
	particles[particleid]  = particle;
}
