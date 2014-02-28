// header is included here

layout (local_size_x = HALFBLOCKSIZE) in;

layout (std430, binding = 0) buffer Data
{
	vec4 data[];
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

#define NUM_BANKS 32
#define LOG_NUM_BANKS 5
#define CONFLICT_FREE_OFFSET(n) ((n) >> LOG_NUM_BANKS)

shared uvec4 mask[BLOCKSIZE + CONFLICT_FREE_OFFSET(BLOCKSIZE)];

const int n = BLOCKSIZE;

uniform int bitshift;

uint GetHash (in vec3 pos)
{
	ivec3 grid = ivec3 (clamp (pos.xyz, vec3 (0, 0, 0), GRID_SIZE));
	return uint (dot (grid, GRID_HASHWEIGHTS));
}

void main (void)
{
	const int gid = int (gl_GlobalInvocationID.x);
	const int lid = int (gl_LocalInvocationIndex);
	
	int ai = lid;
	int bi = lid + (n/2);
	
	int bankOffsetA = CONFLICT_FREE_OFFSET(ai);
	int bankOffsetB = CONFLICT_FREE_OFFSET(bi);
	
	uint bits1 = bitfieldExtract (GetHash (data[gl_WorkGroupID.x * BLOCKSIZE + lid].xyz), bitshift, 2);
	uint bits2 = bitfieldExtract (GetHash (data[gl_WorkGroupID.x * BLOCKSIZE + lid + (n/2)].xyz), bitshift, 2);
	mask[ai + bankOffsetA] = uvec4 (equal (bits1 * uvec4 (1, 1, 1, 1), uvec4 (0, 1, 2, 3)));
	mask[bi + bankOffsetB] = uvec4 (equal (bits2 * uvec4 (1, 1, 1, 1), uvec4 (0, 1, 2, 3)));

	int offset = 1;	
	for (int d = n >> 1; d > 0; d >>= 1)
	{
		barrier ();
		memoryBarrierShared ();
		
		if (lid < d)
		{
			int ai = offset * (2 * lid + 1) - 1;
			int bi = offset * (2 * lid + 2) - 1;
			ai += CONFLICT_FREE_OFFSET(ai);
			bi += CONFLICT_FREE_OFFSET(bi);

			mask[bi] += mask[ai];
		}
		offset *= 2;
	}

	barrier ();
	memoryBarrierShared ();
	
	if (lid == 0)
	{
		for (int i = 0; i < 4; i++)
		{
			blocksum[blocksumoffsets[i] + gl_WorkGroupID.x] = uvec4 (mask[n - 1 + CONFLICT_FREE_OFFSET(n - 1)])[i];
		}

		mask[n - 1 + CONFLICT_FREE_OFFSET(n - 1)] = uvec4 (0, 0, 0, 0);
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
			ai += CONFLICT_FREE_OFFSET(ai);
			bi += CONFLICT_FREE_OFFSET(bi);
			
			uvec4 tmp = mask[ai];
			mask[ai] = mask[bi];
			mask[bi] += tmp;
		}
	}
	
	barrier ();
	memoryBarrierShared ();
	
	prefixsum[gl_WorkGroupID.x * BLOCKSIZE + lid] = uvec4 (mask[ai + bankOffsetA])[bits1];
	prefixsum[gl_WorkGroupID.x * BLOCKSIZE + lid + (n/2)] = uvec4 (mask[bi + bankOffsetB])[bits2];
}
