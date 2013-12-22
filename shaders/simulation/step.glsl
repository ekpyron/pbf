#version 430 core

layout (local_size_x = 8, local_size_y = 8) in;

#define GRID_WIDTH	128
#define GRID_HEIGHT	16
#define GRID_DEPTH 128

const vec3 GRID_SIZE = vec3 (GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH);

const float rho_0 = 1.41;

struct ParticleInfo
{
	vec3 position;
	vec3 velocity;
};

layout (std430, binding = 0) buffer ParticleBuffer
{
	coherent ParticleInfo particles[];
};

layout (std430, binding = 3) buffer LambdaBuffer
{
	coherent float lambdas[];
};

//uniform float timestep;
const float timestep = 0.01;

layout (std430, binding = 1) buffer GridCounters
{
	coherent uint gridcounters[];
};

struct GridCell
{
	uint particleids[8];
};

layout (std430, binding = 2) buffer GridCells
{
	coherent GridCell gridcells[];
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

vec3 gradWspiky (vec3 r)
{
	float l = length (r);
	if (l > 1)
		return vec3 (0, 0, 0);
	float tmp = 1 - l;
	return (-3 * 4.774648292756860 * tmp * tmp) * (r/l);
}

void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.z * gl_NumWorkGroups.y * gl_NumWorkGroups.x
		* gl_WorkGroupSize.x * gl_WorkGroupSize.y
		+ gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x + gl_GlobalInvocationID.x;
	
	ParticleInfo particle = particles[particleid];
	
	// gravity
	particle.velocity += 1000 * vec3 (0, -1, 0) * timestep;
	
	// save old position and predict new position
	vec3 oldposition = particle.position;
	particle.position += timestep * particle.velocity;

	// compute grid id as hash value
	ivec3 grid;
	grid.x = clamp (int (floor (particle.position.x)), 0, GRID_WIDTH);
	grid.y = clamp (int (floor (particle.position.y)), 0, GRID_HEIGHT);
	grid.z = clamp (int (floor (particle.position.z)), 0, GRID_DEPTH);
	
	int gridid = grid.z * GRID_WIDTH * GRID_HEIGHT + grid.y * GRID_WIDTH + grid.x;

	uint pos = atomicAdd (gridcounters[gridid], 1);
	if (pos < 8)
		gridcells[gridid].particleids[pos] = particleid;

	barrier ();
	memoryBarrierBuffer ();
	
	uint neighbours[8*27];
	uint num_neighbours = 0;
	
	uint i;
	
	ivec3 gridoffset;
	for (gridoffset.x = -1; gridoffset.x <= 1; gridoffset.x++)
	for (gridoffset.y = -1; gridoffset.y <= 1; gridoffset.y++)
	for (gridoffset.z = -1; gridoffset.z <= 1; gridoffset.z++)
	{
		ivec3 ngrid = grid + gridoffset;
		int ngridid = ngrid.z * GRID_WIDTH * GRID_HEIGHT + ngrid.y * GRID_WIDTH + ngrid.x;
		for (uint j = 0; j < gridcounters[ngridid]; j++)
		{
			uint id = gridcells[ngridid].particleids[j];
			if (id == particleid)
				i = num_neighbours;
			neighbours[num_neighbours] = id;
			num_neighbours++;
		}
	}
	
	for (uint solver = 0; solver < 1; solver++) {

	barrier ();
	memoryBarrierBuffer ();
	
	float sum_k_grad_Ci = 0;
	float lambda = 0;
	
	float rho = 0;
	
	float scorr = 0;
	for (uint j = 0; j < num_neighbours; j++)
	{
		float len = length (particle.position - particles[neighbours[j]].position);
		float tmp = Wpoly6 (len);
		rho += tmp;
		tmp = -tmp / Wpoly6 (0.1);
		tmp *= tmp;
		scorr += -0.1 * tmp * tmp;
	}
	
	for (uint k = 0; k < num_neighbours; k++)
	{
		vec3 grad_pk_Ci = vec3 (0, 0, 0);
		if (i == k)
		{
			for (uint j = 0; j < num_neighbours; j++)
			{
				grad_pk_Ci += gradWspiky (particle.position - particles[neighbours[j]].position); 
			}
		}
		else
		{
			grad_pk_Ci = -gradWspiky (particle.position - particles[neighbours[k]].position);
		}
		grad_pk_Ci /= rho_0;
		sum_k_grad_Ci += dot (grad_pk_Ci, grad_pk_Ci);
	}

	float C_i = rho / rho_0 - 1;
	lambda = -C_i / (sum_k_grad_Ci + 0.01);
	
	lambdas[particleid] = lambda;
	
	barrier ();
	memoryBarrierBuffer ();
	
	vec3 deltap = vec3 (0, 0, 0);
	
	for (uint j = 0; j < num_neighbours; j++)
	{
		deltap += (lambda + lambdas[neighbours[j]]) * gradWspiky (particle.position - particles[neighbours[j]].position);
	}
	
	particle.position += deltap / rho_0;
	particle.position = clamp (particle.position, vec3 (0, 0, 0), GRID_SIZE);
	barrier ();
	particles[particleid].position = particle.position;
	}

//	vec3 flipvelocity = vec3 (greaterThanEqual (particle.position, vec3 (0, 0, 0))) * vec3(lessThanEqual (particle.position, GRID_SIZE));
	particle.velocity = 0.5 * (particle.position - oldposition) / timestep;
//	particle.velocity *= -2 * (flipvelocity - 0.5);
/*
	vec3 v_i = particle.velocity;
	for (uint j = 0; j < num_neighbours; j++)
	{
		vec3 v_j = particles[neighbours[j]].velocity;
		particle.velocity += 0.01 * (v_i - v_j) * Wpoly6 (particle.position - particles[neighbours[j]].position);
	}*/

//	barrier ();
	
	particles[particleid].velocity = particle.velocity;
}
