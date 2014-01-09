#version 430 core

layout (local_size_x = 8, local_size_y = 8) in;

#define GRID_WIDTH	128
#define GRID_HEIGHT	16
#define GRID_DEPTH 128

const vec3 GRID_SIZE = vec3 (GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH);

#define MAX_GRID_ID (GRID_WIDTH * GRID_HEIGHT * GRID_DEPTH - 1)

const float rho_0 = 1;

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
	return (-3 * 4.774648292756860 * tmp * tmp) * r;
}

// offsets to find neighbouring cells in one dimensional array
const int gridoffsets[27] = {
	-GRID_WIDTH * GRID_HEIGHT - GRID_WIDTH - 1,		// -1, -1, -1
	-GRID_WIDTH * GRID_HEIGHT - GRID_WIDTH,			// -1, -1, 0
	-GRID_WIDTH * GRID_HEIGHT - GRID_WIDTH + 1,		// -1, -1, 1
	-GRID_WIDTH * GRID_HEIGHT - 1,					// -1, 0, -1
	-GRID_WIDTH * GRID_HEIGHT,						// -1, 0, 0
	-GRID_WIDTH * GRID_HEIGHT + 1,					// -1, 0, 1
	-GRID_WIDTH * GRID_HEIGHT + GRID_WIDTH - 1,		// -1, 1, -1
	-GRID_WIDTH * GRID_HEIGHT + GRID_WIDTH,			// -1, 1, 0
	-GRID_WIDTH * GRID_HEIGHT + GRID_WIDTH + 1,		// -1, 1, 1
	-GRID_WIDTH - 1,								// 0, -1, -1
	-GRID_WIDTH,									// 0, -1, 0
	-GRID_WIDTH + 1,								// 0, -1, 1
	-1,												// 0, 0, -1
	0,												// 0, 0, 0
	1,												// 0, 0, 1
	GRID_WIDTH - 1,									// 0, 1, -1
	GRID_WIDTH,										// 0, 1, 0
	GRID_WIDTH + 1,									// 0, 1, 1
	GRID_WIDTH * GRID_HEIGHT - GRID_WIDTH - 1,		// 1, -1, -1
	GRID_WIDTH * GRID_HEIGHT - GRID_WIDTH,			// 1, -1, 0
	GRID_WIDTH * GRID_HEIGHT - GRID_WIDTH + 1,		// 1, -1, 1
	GRID_WIDTH * GRID_HEIGHT - 1,					// 1, 0, -1
	GRID_WIDTH * GRID_HEIGHT,						// 1, 0, 0
	GRID_WIDTH * GRID_HEIGHT + 1,					// 1, 0, 1
	GRID_WIDTH * GRID_HEIGHT + GRID_WIDTH - 1,		// 1, 1, -1
	GRID_WIDTH * GRID_HEIGHT + GRID_WIDTH,			// 1, 1, 0
	GRID_WIDTH * GRID_HEIGHT + GRID_WIDTH + 1		// 1, 1, 1
};

// convenience macros to iterate over all neighbouring particles
// within the loop the given variable is defined to contain the index
// of the current neighbour particle in the particles[] array
#define FOR_EACH_NEIGHBOUR(var) for (int o = 0; o < 27; o++) {\
		int ngridid = gridid + gridoffsets[o];\
		if (ngridid < 0 || ngridid > MAX_GRID_ID)\
			continue;\
		for (uint gridparticle = 0; gridparticle < gridcounters[ngridid]; gridparticle++) {\
			uint var = gridcells[ngridid].particleids[gridparticle];
#define END_FOR_EACH_NEIGHBOUR 	}}

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
	
	for (uint solver = 0; solver < 1; solver++) {

	barrier ();
	memoryBarrierBuffer ();
	
	float sum_k_grad_Ci = 0;
	float lambda = 0;
	float rho = 0;
	float scorr = 0;

	vec3 grad_pi_Ci = vec3 (0, 0, 0);
	FOR_EACH_NEIGHBOUR(j)
	{			
		// compute rho_i (equation 2)
		float len = length (particle.position - particles[j].position);
		float tmp = Wpoly6 (len);
		rho += tmp;
	
		// compute scorr (equation 13) - currently unused
		tmp = -tmp / Wpoly6 (0.1);
		tmp *= tmp;
		scorr += -0.1 * tmp * tmp;
			
		// sum gradients of Ci (equation 8 and parts of equation 9)
		// use j as k so that we can stay in the same loop
		uint k = j;
		// for k=i it holds that particle.position==particles[k].position, so
		// the summand does no harm
		vec3 grad_pk_Ci = vec3 (0, 0, 0);
		grad_pk_Ci = gradWspiky (particle.position - particles[k].position);
		grad_pk_Ci /= rho_0;
		sum_k_grad_Ci += dot (grad_pk_Ci, grad_pk_Ci);
		
		// now use j as j again and accumulate grad_pi_Ci for the case k=i
		// from equation 8
		grad_pi_Ci += gradWspiky (particle.position - particles[j].position); 
	}
	END_FOR_EACH_NEIGHBOUR
	// add grad_pi_Ci to the sum
	sum_k_grad_Ci += dot (grad_pi_Ci, grad_pi_Ci) / rho_0;
	
	// compute lambda_i (equations 1 and 9)
	float C_i = rho / rho_0 - 1;
	lambda = -C_i / (sum_k_grad_Ci + 0.01);
	
	lambdas[particleid] = lambda;
	
	barrier ();
	memoryBarrierBuffer ();
	
	vec3 deltap = vec3 (0, 0, 0);
	
	FOR_EACH_NEIGHBOUR(j)
	{
		// accumulate position corrections (part of equation 12)
		deltap += (lambda + lambdas[j]) * gradWspiky (particle.position - particles[j].position);
	}
	END_FOR_EACH_NEIGHBOUR

	particle.position += deltap / rho_0;
	
	// collision detection begin
	particle.position = clamp (particle.position, vec3 (0, 0, 0), GRID_SIZE);
	// collision detection end
	
	barrier ();
	particles[particleid].position = particle.position;
	}

	particles[particleid].velocity = 0.5 * (particle.position - oldposition) / timestep;
}
