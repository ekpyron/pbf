// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

layout (std430, binding = 0) writeonly buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

uniform bool extforce;

layout (binding = 0) uniform samplerBuffer positiontexture;
layout (binding = 1) uniform samplerBuffer velocitytexture;


void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.x;
	
	vec3 position = texelFetch (positiontexture, int (particleid)).xyz;
	vec3 velocity = texelFetch (velocitytexture, int (particleid)).xyz;
	
	// optionally apply an additional external force to some particles
	if (extforce && position.z > GRID_DEPTH/2)
		velocity += vec3 (0, 0, -20) * timestep;
	
	// gravity
	velocity += gravity * vec3 (0, -1, 0) * timestep;
	
	// predict new position
	particlekeys[particleid].position = position + timestep * velocity;
	particlekeys[particleid].id = particleid;
	// set color
	// particles[particleid].color = vec3 (0.25, 0, 1);
}
