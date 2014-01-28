// simulation/include.glsl is included here

#define BLOCKSIZE 512
#define HALFBLOCKSIZE 256

layout (local_size_x = HALFBLOCKSIZE) in;

struct ParticleInfo
{
	vec3 position;
	vec4 oldposition;
};

layout (std430, binding = 0) buffer Data
{
	ParticleInfo data[];
};

layout (std430, binding = 1) writeonly buffer PrefixSum
{
	uint prefixsum[];
};

layout (std430, binding = 2) writeonly buffer BlockSum
{
	uint blocksum[];
};

uniform uvec4 blocksumoffsets;

shared uvec4 mask[BLOCKSIZE];
shared uvec4 sblocksum;

const int n = BLOCKSIZE;

uniform int bitshift;

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
	const int gid = int (gl_GlobalInvocationID.x);
	const int lid = int (gl_LocalInvocationIndex);
	
	uint bits1 = (GetHash (data[2 * gid].position) & (3 << bitshift)) >> bitshift;
	uint bits2 = (GetHash (data[2 * gid + 1].position) & (3 << bitshift)) >> bitshift;
	mask[2 * lid] = uvec4 (equal (bits1 * uvec4 (1, 1, 1, 1), uvec4 (0, 1, 2, 3)));
	mask[2 * lid + 1] = uvec4 (equal (bits2 * uvec4 (1, 1, 1, 1), uvec4 (0, 1, 2, 3)));

	int offset = 1;	
	for (int d = n >> 1; d > 0; d >>= 1)
	{
		barrier ();
		memoryBarrierShared ();
		
		if (lid < d)
		{
			int ai = offset * (2 * lid + 1) - 1;
			int bi = offset * (2 * lid + 2) - 1;

			mask[bi] += mask[ai];
		}
		offset *= 2;
	}

	barrier ();
	memoryBarrierShared ();
	
	if (lid == 0)
	{
		uvec4 tmp;
		tmp.x = 0;
		tmp.y = mask[n - 1].x;
		tmp.z = tmp.y + mask[n - 1].y;
		tmp.w = tmp.z + mask[n - 1].z;
		sblocksum = tmp;

		for (int i = 0; i < 4; i++)
		{
			blocksum[blocksumoffsets[i] + gl_WorkGroupID.x] = mask[n - 1][i];
		}

		mask[n - 1] = uvec4 (0, 0, 0, 0);
	}
	
	
	for (int d = 1; d < n; d *= 2)
	{
		offset >>= 1;
		barrier ();
		memoryBarrierShared ();
		
		if (lid < d)
		{
			int ai = offset * (2 * lid + 1) - 1;
			int bi = offset * (2 * lid + 2) - 1;
			
			uvec4 tmp = mask[ai];
			mask[ai] = mask[bi];
			mask[bi] += tmp;
		}
	}
	
	barrier ();
	memoryBarrierShared ();
	
	prefixsum[2 * gid] = mask[2 * lid][bits1];
	prefixsum[2 * gid + 1] = mask[2 * lid + 1][bits2];
}
