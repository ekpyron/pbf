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

layout (local_size_x = BLOCKSIZE) in;

layout (std430, binding = 0) readonly buffer Data
{
	vec4 data[];
};

layout (std430, binding = 1) readonly buffer PrefixSum
{
	uint prefixsum[];
};

layout (std430, binding = 2) readonly buffer BlockSum
{
	uint blocksum[];
};

layout (std430, binding = 3) writeonly buffer Result
{
	vec4 result[];
};

uniform uvec4 blocksumoffsets;

uniform int bitshift;

uint GetHash (int id)
{
	vec3 pos = data[id].xyz;
	ivec3 grid = ivec3 (clamp (pos, vec3 (0, 0, 0), GRID_SIZE));
	return uint (dot (grid, GRID_HASHWEIGHTS));
}

void main (void)
{
	const int gid = int (gl_GlobalInvocationID.x);
	const int lid = int (gl_LocalInvocationIndex);

	uint bits = bitfieldExtract (GetHash (gid), bitshift, 2);
	
	result[blocksum[blocksumoffsets[bits] + gl_WorkGroupID.x] + prefixsum[gid]] = data[gid];
}
