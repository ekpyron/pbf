// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

layout (std430, binding = 0) writeonly buffer ParticleKeys
{
	vec4 particlekeys[];
};

uniform bool extforce;

layout (binding = 0) uniform samplerBuffer positiontexture;
layout (binding = 1) uniform samplerBuffer velocitytexture;


void main (void)
{
	int particleid = int (gl_GlobalInvocationID.x);
	
	vec3 position = texelFetch (positiontexture, particleid).xyz;
	vec3 velocity = texelFetch (velocitytexture, particleid).xyz;
	
	// optionally apply an additional external force to some particles
	if (extforce && position.z > GRID_DEPTH/2)
		velocity += vec3 (0, 0, -20) * timestep;
	
	// gravity
	velocity += gravity * vec3 (0, -1, 0) * timestep;
	
	// predict new position
	particlekeys[particleid].xyz = position + timestep * velocity;
	particlekeys[particleid].w = intBitsToFloat (particleid);
	// set color
	// particles[particleid].color = vec3 (0.25, 0, 1);
}
