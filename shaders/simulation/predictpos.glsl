// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

struct ParticleInfo
{
	vec3 position;
	vec3 oldposition;
};

layout (std430, binding = 0) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

layout (std430, binding = 1) writeonly buffer AuxBuffer
{
	vec4 auxdata[];
};

void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.x;

	// highlight a particle	
	if (particleid == highlightparticle)
		auxdata[particleid] = vec4 (1, 0, 0, 1);
	
	ParticleInfo particle = particles[particleid];
	
	vec3 velocity = (particle.position - particle.oldposition) / timestep;
	
	// gravity
	velocity += gravity * vec3 (0, -1, 0) * timestep;
	
	// save old position and predict new position
	particle.oldposition = particle.position;
	particle.position += timestep * velocity;
	particles[particleid]  = particle;
}
