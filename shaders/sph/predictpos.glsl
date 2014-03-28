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

layout (std430, binding = 0) writeonly buffer ParticleKeys
{
	vec4 particlekeys[];
};

uniform bool extforce;

layout (binding = 0) uniform samplerBuffer positiontexture;
layout (binding = 1) uniform samplerBuffer velocitytexture;


void main (void)
{
	int particleid = int (gl_GlobalInvocationID.x);
	
	vec3 position = texelFetch (positiontexture, particleid).xyz;
	vec3 velocity = texelFetch (velocitytexture, particleid).xyz;
	
	// optionally apply an additional external force to some particles
	if (extforce && position.z > GRID_SIZE.z/2)
		velocity += vec3 (0, 0, -20) * timestep;
	
	// gravity
	velocity += gravity * vec3 (0, -1, 0) * timestep;
	
	// predict new position
	particlekeys[particleid].xyz = position + timestep * velocity;
	particlekeys[particleid].w = intBitsToFloat (particleid);
	// set color
	// particles[particleid].color = vec3 (0.25, 0, 1);
}
