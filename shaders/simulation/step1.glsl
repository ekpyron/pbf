#version 430 core

layout (local_size_x = 16, local_size_y = 16) in;

#define GRID_WIDTH	64
#define GRID_HEIGHT	64
#define GRID_DEPTH 64

struct ParticleInfo
{
	vec3 position;
	vec3 oldposition;
};

layout (std430, binding = 0) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

//uniform float timestep;
const float timestep = 0.01;
const float lasttimestep = 0.01;

layout (std430, binding = 1) buffer GridCounters
{
	uint gridcounters[];
};

struct GridCell
{
	ParticleInfo particleids[8];
};

layout (std430, binding = 2) buffer GridCells
{
	GridCell gridcells[];
};

void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.z * gl_NumWorkGroups.y * gl_NumWorkGroups.x * 256
		+ gl_GlobalInvocationID.y * gl_NumWorkGroups.x * 16 + gl_GlobalInvocationID.x;
	
	ParticleInfo particle = particles[particleid];
	
	// gravity
	vec3 velocity = 0.95 * (particle.position - particle.oldposition) / lasttimestep;
	velocity += 1000 * vec3 (0, -1, 0) * timestep;
	
	// save old position and predict new position
	particle.oldposition = particle.position;
	particle.position += timestep * velocity;

	// compute grid id as hash value
	ivec3 grid;
	grid.x = clamp (int (floor (particle.position.x)), 0, GRID_WIDTH);
	grid.y = clamp (int (floor (particle.position.y)), 0, GRID_HEIGHT);
	grid.z = clamp (int (floor (particle.position.z)), 0, GRID_DEPTH);
	
	int gridid = grid.z * GRID_WIDTH * GRID_HEIGHT + grid.y * GRID_WIDTH + grid.x;

	uint pos = atomicAdd (gridcounters[gridid], 1);
	if (pos < 8)
		gridcells[gridid].particleids[pos] = particle;
		
	barrier ();
	memoryBarrierBuffer ();
	
	particle.position = clamp (particle.position, vec3 (0, 0, 0), vec3 (GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH));

	ivec3 gridoffset;
	for (gridoffset.x = -1; gridoffset.x <= 1; gridoffset.x++)
	for (gridoffset.y = -1; gridoffset.y <= 1; gridoffset.y++)
	for (gridoffset.z = -1; gridoffset.z <= 1; gridoffset.z++)
	{
		ivec3 ngrid = grid + gridoffset;
		
		int ngridid = ngrid.z * GRID_WIDTH * GRID_HEIGHT + ngrid.y * GRID_WIDTH + ngrid.x;
		
		for (uint i = 0; i < gridcounters[ngridid]; i++)
		{
			if (gridoffset == ivec3 (0, 0, 0) && i == pos)
				continue;
			
			ParticleInfo neighbour = gridcells[ngridid].particleids[i];
			
			vec3 impactdir = neighbour.position - particle.position;
			float impactlen = length (impactdir);
			if (impactlen < 1)
			{
				// collisiion
				impactdir /= impactlen;
				particle.position = 0.5 * (particle.position + neighbour.position - impactdir);
			}
		}
	}
		
	barrier ();
	
	particles[particleid] = particle;
}
