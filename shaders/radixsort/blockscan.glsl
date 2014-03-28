/*
 * Copyright (c) 2013-2014 Daniel Kirchner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE ANDNONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
// header is included here

layout (local_size_x = HALFBLOCKSIZE) in;

layout (std430, binding = 0) buffer Data
{
	uint data[];
};

layout (std430, binding = 1) writeonly buffer BlockSums
{
	uint blocksums[];
};

#define NUM_BANKS 32
#define LOG_NUM_BANKS 5
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
	
	temp[ai + bankOffsetA] = data[gl_WorkGroupID.x * BLOCKSIZE + lid];
	temp[bi + bankOffsetB] = data[gl_WorkGroupID.x * BLOCKSIZE + lid + (n/2)];
	
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
	
	data[gl_WorkGroupID.x * BLOCKSIZE + lid] = temp[ai + bankOffsetA];
	data[gl_WorkGroupID.x * BLOCKSIZE + lid + (n/2)] = temp[bi + bankOffsetB];
}
