// include.glsl is included here
#line 3

layout (local_size_x = 16, local_size_y = 16) in;

struct ParticleInfo
{
	vec3 position;
	vec3 oldposition;
};

layout (std430, binding = 0) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

layout (std430, binding = 4) buffer AuxBuffer
{
	vec4 auxdata[];
};

layout (std430, binding = 1) buffer GridCounters
{
	uint gridcounters[];
};

struct GridCell
{
	uint particleids[64];
};

layout (std430, binding = 2) writeonly buffer GridCells
{
	GridCell gridcells[];
};

void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.z * gl_NumWorkGroups.y * gl_NumWorkGroups.x
		* gl_WorkGroupSize.x * gl_WorkGroupSize.y
		+ gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x + gl_GlobalInvocationID.x;

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

	// compute grid id as hash value
	ivec3 grid;
	grid.x = clamp (int (floor (particle.position.x)), 0, GRID_WIDTH);
	grid.y = clamp (int (floor (particle.position.y)), 0, GRID_HEIGHT);
	grid.z = clamp (int (floor (particle.position.z)), 0, GRID_DEPTH);
	
	int gridid = grid.y * GRID_WIDTH * GRID_DEPTH + grid.z * GRID_WIDTH + grid.x;

	uint pos = atomicAdd (gridcounters[gridid], 1);
	if (pos < 64)
		gridcells[gridid].particleids[pos] = particleid;
}
