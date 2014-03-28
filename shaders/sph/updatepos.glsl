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

layout (std430, binding = 0) buffer ParticleKeys
{
	vec4 particlekeys[];
};

layout (binding = 2) uniform isamplerBuffer neighbourcelltexture;
layout (binding = 3) uniform samplerBuffer lambdatexture;

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
	if (l > h || l == 0)
		return vec3 (0, 0, 0);
	float tmp = h - l;
	return (-3 * 4.774648292756860 * tmp * tmp) * r / (l * h*h*h*h*h*h);
}

#define FOR_EACH_NEIGHBOUR(var) for (int o = 0; o < 3; o++) {\
		ivec3 datav = texelFetch (neighbourcelltexture, int (gl_GlobalInvocationID.x * 3 + o)).xyz;\
		for (int comp = 0; comp < 3; comp++) {\
		int data = datav[comp];\
		int entries = data >> 24;\
		data = data & 0xFFFFFF;\
		if (data == 0) continue;\
		for (int var = data; var < data + entries; var++) {\
		if (var != gl_GlobalInvocationID.x) {
#define END_FOR_EACH_NEIGHBOUR(var)	}}}}

void main (void)
{
	vec3 position = particlekeys[gl_GlobalInvocationID.x].xyz;

	vec3 deltap = vec3 (0, 0, 0);
	
	//float lambda = lambdas[gl_GlobalInvocationID.x];
	float lambda = texelFetch (lambdatexture, int (gl_GlobalInvocationID.x)).x;
			
	FOR_EACH_NEIGHBOUR(j)
	{
		// This might fetch an already updated position,
		// but that doesn't cause any harm.
		vec3 position_j = particlekeys[j].xyz;
		
		float scorr = tensile_instability_scale * Wpoly6 (distance (position, position_j));
		scorr *= scorr;
		scorr *= scorr;
		scorr = -tensile_instability_k * scorr;
		
		float lambda_j = texelFetch (lambdatexture, j).x;  
	
		// accumulate position corrections (part of equation 12)
		deltap += (lambda + lambda_j + scorr) * gradWspiky (position - position_j);
	}
	END_FOR_EACH_NEIGHBOUR(j)

	position += one_over_rho_0 * deltap;

	// collision detection begin
	vec3 wall = vec3 (16, 0, 16);
	position = clamp (position, vec3 (0, 0, 0) + wall, GRID_SIZE - wall);
	// collision detection end
	
	particlekeys[gl_GlobalInvocationID.x].xyz = position;
}
