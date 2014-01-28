// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

struct ParticleInfo
{
	vec3 position;
	vec4 oldposition;
};

layout (std430, binding = 0) readonly buffer ParticleBuffer
{
	ParticleInfo particles[];
};

layout (std430, binding = 3) writeonly buffer GridBuffer
{
	int grid[];
};

layout (binding = 0, r8i) uniform writeonly iimageBuffer flagtexture;

uint GetHash (in vec3 pos)
{
	ivec3 grid;
	grid.x = clamp (int (floor (pos.x)), 0, GRID_WIDTH);
	grid.y = clamp (int (floor (pos.y)), 0, GRID_HEIGHT);
	grid.z = clamp (int (floor (pos.z)), 0, GRID_DEPTH);
	
	return grid.y * GRID_WIDTH * GRID_DEPTH + grid.z * GRID_WIDTH + grid.x;
}

void main (void)
{
	uint gid;
	gid = gl_GlobalInvocationID.x;
	
	if (gid == 0)
	{
		grid[0] = 0;
		imageStore (flagtexture, 0, ivec4 (0, 0, 0, 0));
		imageStore (flagtexture, int (NUM_PARTICLES), ivec4 (1, 0, 0, 0));
		return;
	}

	uint hash = GetHash (particles[gid].position);
	
	if (hash != GetHash (particles[gid - 1].position))
	{
		grid[hash] = int (gid);
		imageStore (flagtexture, int (gid), ivec4 (1, 0, 0, 0));
	}
	else
	{
		imageStore (flagtexture, int (gid), ivec4 (0, 0, 0, 0));
	}
}
