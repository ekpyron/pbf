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

/**
 * This shader computes the block-wise prefix sums counting the occurrences of the value of two bits (therefore in [0, 3]).
 * The result is stored in the prefixsum array.
 * The total sum of each block is stored in the blocksum array.
 */

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

	// index in the lower half of the block
	int ai = lid;
	// index in the upper half of the block
	int bi = lid + (n/2);
	
	int bankOffsetA = CONFLICT_FREE_OFFSET(ai);
	int bankOffsetB = CONFLICT_FREE_OFFSET(bi);

	// the 2 bits of the hash value at position bitshift, resp. in the lower and upper half of the block
	// values 0 <= x < 4
	uint bits1 = bitfieldExtract (GetHash (data[gl_WorkGroupID.x * BLOCKSIZE + ai].xyz), bitshift, 2);
	uint bits2 = bitfieldExtract (GetHash (data[gl_WorkGroupID.x * BLOCKSIZE + bi].xyz), bitshift, 2);

	// mask containing a single component with a 1 corresponding to the value of bits{1,2}
	mask[ai + bankOffsetA] = uvec4 (equal (bits1 * uvec4 (1, 1, 1, 1), uvec4 (0, 1, 2, 3)));
	mask[bi + bankOffsetB] = uvec4 (equal (bits2 * uvec4 (1, 1, 1, 1), uvec4 (0, 1, 2, 3)));

    // build prefix sums of mask
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
		    // final entry of prefix sum mask with total sum
			blocksum[blocksumoffsets[i] + gl_WorkGroupID.x] = uvec4 (mask[n - 1 + CONFLICT_FREE_OFFSET(n - 1)])[i];
		}

        // reset total sum to 0,0,0,0
		mask[n - 1 + CONFLICT_FREE_OFFSET(n - 1)] = uvec4 (0, 0, 0, 0);
	}
	
	// shift entries from right to left s.t. the (0,0,0,0) is at the first position and mask contains a prefix sum
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
