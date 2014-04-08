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

layout (std430, binding = 0) readonly buffer ParticleKeys
{
	vec4 particlekeys[];
};

layout (binding = 0, r32i) uniform writeonly iimage3D gridtexture;
layout (binding = 1, r32i) uniform writeonly iimage3D gridendtexture;

void main (void)
{
	uint gid;
	gid = gl_GlobalInvocationID.x;
	
	if (gid == 0)
	{
		imageStore (gridtexture, ivec3 (0, 0, 0), ivec4 (0, 0, 0, 0));
		return;
	}

	ivec3 gridpos = ivec3 (clamp (particlekeys[gid].xyz, vec3 (0, 0, 0), GRID_SIZE));
	ivec3 gridpos2 = ivec3 (clamp (particlekeys[gid - 1].xyz, vec3 (0, 0, 0), GRID_SIZE));
	
	if (gridpos != gridpos2)
	{
		imageStore (gridtexture, gridpos, ivec4 (gid, 0, 0, 0));
		imageStore (gridendtexture, gridpos2, ivec4 (gid, 0, 0, 0));
	}
}
