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

float Wpoly6 (float r)
{
	if (r > 1)
		return 0;
	float tmp = 1 - r * r;
	return 1.56668147106 * tmp * tmp * tmp;
}

float Wspiky (float r)
{
	if (r > 1)
		return 0;
	float tmp = 1 - r;
	return 4.774648292756860 * tmp * tmp * tmp;
}

float gradWspiky (float r)
{
	if (r > 1)
		return 0;
	float tmp = 1 - r;
	return -3 * 4.774648292756860 * tmp * tmp;
}

void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.z * gl_NumWorkGroups.y * gl_NumWorkGroups.x * 256
		+ gl_GlobalInvocationID.y * gl_NumWorkGroups.x * 16 + gl_GlobalInvocationID.x;
	
	ParticleInfo particle = particles[particleid];
	
	// gravity
	vec3 velocity = (particle.position - particle.oldposition) / lasttimestep;
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
	
	vec3 neighbours[8*27 + 1];
	float sum_k_grad_Ci = 0;
	float lambda_i[8*27 + 1];
	uint num_neighbours = 1;
	
	neighbours[0] = particle.position;
	lambda_i[0] = 0;
	const float rho_0 = 1;
	
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
			
			vec3 pos = gridcells[ngridid].particleids[i].position;
			
			neighbours[num_neighbours] = pos;
			lambda_i[num_neighbours] = 0;
			num_neighbours++;
		}
	}
	
	for (uint i = 0; i < num_neighbours; i++)
	{
		for (uint j = 0; j < num_neighbours; j++)
		{
			float len = length (neighbours[i] - neighbours[j]);
			lambda_i[i] += Wpoly6 (len);
		}
	}
	
	for (uint i = 0; i < num_neighbours; i++)
	{
		for (uint k = 0; k < num_neighbours; k++)
		{
			float grad_pk_Ci = 0;
			if (i == k)
			{
				for (uint j = 0; j < num_neighbours; j++)
				{
					grad_pk_Ci += gradWspiky (length (neighbours[i] - neighbours[j])); 
				}
			}
			else
			{
				grad_pk_Ci = -gradWspiky (length (neighbours[i] - neighbours[k]));
			}
			sum_k_grad_Ci += grad_pk_Ci * grad_pk_Ci;
		}
	}
	
	sum_k_grad_Ci /= rho_0;
	for (uint i = 0; i < num_neighbours; i++)
	{	
		float C_i = lambda_i[i] / rho_0 - 1;
		lambda_i[i] = -C_i / (sum_k_grad_Ci + 0.01);
	}
	
	vec3 deltap = vec3 (0, 0, 0);
	
	for (uint i = 0; i < num_neighbours; i++)
	{
		for (uint j = 0; j < num_neighbours; j++)
		{
			deltap += (lambda_i[i] + lambda_i[j]) * gradWspiky (length (neighbours[i] - neighbours[j]));
		}
	}
	
	particle.position += deltap;					
	particle.position = clamp (particle.position, vec3 (0, 0, 0), vec3 (GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH));
	
		
	barrier ();
	
	particles[particleid] = particle;
	
}
