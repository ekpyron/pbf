// include.glsl is included here
#line 3

layout (local_size_x = 8, local_size_y = 8) in;

struct ParticleInfo
{
	vec3 position;
	vec3 oldposition;
};

layout (std430, binding = 0) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

layout (std430, binding = 3) readonly buffer LambdaBuffer
{
	float lambdas[];
};

layout (std430, binding = 4) buffer AuxBuffer
{
	vec4 auxdata[];
};

layout (std430, binding = 1) readonly buffer GridCounters
{
	uint gridcounters[];
};

struct GridCell
{
	uint particleids[64];
};

layout (std430, binding = 2) readonly buffer GridCells
{
	GridCell gridcells[];
};

float Wpoly6 (float r)
{
	if (r > h)
		return 0;
	float tmp = h * h - r * r;
	return 1.56668147106 * tmp * tmp * tmp / (h*h*h*h*h*h*h*h*h);
}

float Wspiky (float r)
{
	if (r > h)
		return 0;
	float tmp = h - r;
	return 4.774648292756860 * tmp * tmp * tmp / (h*h*h*h*h*h);
}

vec3 gradWspiky (vec3 r)
{
	float l = length (r);
	if (l > h)
		return vec3 (0, 0, 0);
	float tmp = h - l;
	return (-3 * 4.774648292756860 * tmp * tmp) * r / (l * h*h*h*h*h*h);
}

const ivec3 gridoffsets[27] = {
	ivec3 (-1, -1, -1),
	ivec3 (-1, -1, 0),
	ivec3 (-1, -1, 1),
	ivec3 (-1, 0, -1),
	ivec3 (-1, 0, 0),
	ivec3 (-1, 0, 1),
	ivec3 (-1, 1, -1),
	ivec3 (-1, 1, 0),
	ivec3 (-1, 1, 1),
	ivec3 (0, -1, -1),
	ivec3 (0, -1, 0),
	ivec3 (0, -1, 1),
	ivec3 (0, 0, -1),
	ivec3 (0, 0, 0),
	ivec3 (0, 0, 1),
	ivec3 (0, 1, -1),
	ivec3 (0, 1, 0),
	ivec3 (0, 1, 1),
	ivec3 (1, -1, -1),
	ivec3 (1, -1, 0),
	ivec3 (1, -1, 1),
	ivec3 (1, 0, -1),
	ivec3 (1, 0, 0),
	ivec3 (1, 0, 1),
	ivec3 (1, 1, -1),
	ivec3 (1, 1, 0),
	ivec3 (1, 1, 1)
};

#define FOR_EACH_NEIGHBOUR(var) for (int o = 0; o < 27; o++) {\
		ivec3 ngrid = grid + gridoffsets[o];\
		if (any (lessThan (ngrid, ivec3 (0, 0, 0))) || any (greaterThanEqual (ngrid, GRID_SIZE)))\
			continue;\
		int ngridid = ngrid.y * GRID_WIDTH * GRID_DEPTH + ngrid.z * GRID_WIDTH + ngrid.x;\
		for (uint gridparticle = 0; gridparticle < gridcounters[ngridid]; gridparticle++) {\
			uint var = gridcells[ngridid].particleids[gridparticle];\
			if (var == particleid)\
				continue;
#define END_FOR_EACH_NEIGHBOUR 	}}

void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.z * gl_NumWorkGroups.y * gl_NumWorkGroups.x
		* gl_WorkGroupSize.x * gl_WorkGroupSize.y
		+ gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x + gl_GlobalInvocationID.x;

	vec3 position = particles[particleid].position;
	
	// compute grid id as hash value
	ivec3 grid;
	grid.x = clamp (int (floor (position.x)), 0, GRID_WIDTH);
	grid.y = clamp (int (floor (position.y)), 0, GRID_HEIGHT);
	grid.z = clamp (int (floor (position.z)), 0, GRID_DEPTH);
	
	int gridid = grid.y * GRID_WIDTH * GRID_DEPTH + grid.z * GRID_WIDTH + grid.x;

	vec3 deltap = vec3 (0, 0, 0);
	
	float lambda = lambdas[particleid];
		
	FOR_EACH_NEIGHBOUR(j)
	{
		// accumulate position corrections (part of equation 12)
		deltap += (lambda + lambdas[j]) * gradWspiky (position - particles[j].position);
	}
	END_FOR_EACH_NEIGHBOUR

	position += deltap / rho_0;

	// collision detection begin
	position = clamp (position, vec3 (0, 0, 0), GRID_SIZE);
	// collision detection end
	
	particles[particleid].position = position;
}
