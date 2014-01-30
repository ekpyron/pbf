// simulation/include.glsl is included here

#define BLOCKSIZE 512
#define HALFBLOCKSIZE 256

layout (local_size_x = HALFBLOCKSIZE) in;

layout (std430, binding = 0) buffer Data
{
	uint data[];
};

layout (std430, binding = 1) writeonly buffer BlockSums
{
	uint blocksums[];
};

#define NUM_BANKS 16
#define LOG_NUM_BANKS 4
#define CONFLICT_FREE_OFFSET(n) ((n) >> LOG_NUM_BANKS)

shared uint temp[BLOCKSIZE + CONFLICT_FREE_OFFSET(BLOCKSIZE)];

const int n = BLOCKSIZE;

void main (void)
{
	int gid = int (gl_GlobalInvocationID.x);
	int lid = int (gl_LocalInvocationIndex);
	int offset = 1;
	
	int ai = lid;
	int bi = lid + (n/2);
	
	int bankOffsetA = CONFLICT_FREE_OFFSET(ai);
	int bankOffsetB = CONFLICT_FREE_OFFSET(bi);
	
	temp[ai + bankOffsetA] = data[gid];
	temp[bi + bankOffsetB] = data[gid + (n/2)];
	
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
			
			temp[bi] += temp[ai]; 
		}
		offset *= 2;
	}
	
	if (lid == 0)
	{
		blocksums[gl_WorkGroupID.x] = temp[n - 1 + CONFLICT_FREE_OFFSET(n-1)];
		temp[n - 1 + CONFLICT_FREE_OFFSET(n-1)] = 0;
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
			
			uint t = temp[ai];
			temp[ai] = temp[bi];
			temp[bi] += t;
		}
	}
	
	barrier ();
	memoryBarrierShared ();
	
	data[gid] = temp[ai + bankOffsetA];
	data[gid + (n/2)] = temp[bi + bankOffsetB];
}
