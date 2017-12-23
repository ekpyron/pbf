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
//#version 430 core

layout (binding = 0) uniform sampler2D depthtex;

// input from vertex shader
in vec2 fTexcoord;

// blur weights
layout (std430, binding = 0) readonly buffer Weights {
	vec2 weights[];
};

// offset scale indicating input size and blur direction
uniform vec2 offsetscale;

// falloff scaling depth differences in order to cutoff the blur
const float falloff = 1000.0f;

// linearize a depth value in order to determine a meaningful difference
float linearizeDepth (in float d)
{
	const float f = 1000.0f;
	const float n = 1.0f;
	return (2 * n) / (f + n - d * (f - n));
}

void main(void)
{
	// fetch current depth
	float depth = texture (depthtex, fTexcoord).x;
	// discard if nothing was rendered at the given position
	if (depth == 1.0)
		discard;
	
	float result;
	float wsum;
	
	// initialize with weighted current depth
	wsum = weights[0].r;
	result = depth * wsum;
	
	// sum weighted data
	for (int i = 1; i < weights.length (); i++)
	{
		// fetch depth values
		float d1 = texture (depthtex, fTexcoord + weights[i].g * offsetscale).x;
		float d2 = texture (depthtex, fTexcoord - weights[i].g * offsetscale).x;

		// compute cutoff weights from the linearized depth differences 
		float r1 = (linearizeDepth (depth) - linearizeDepth (d1)) * falloff;
		float w1 = exp (-r1 * r1);

		float r2 = (linearizeDepth (depth) - linearizeDepth (d2)) * falloff;
		float w2 = exp (-r2 * r2);
		
		// sum values and weights
		wsum += weights[i].r * (w1 + w2);
		result += weights[i].r * (w1 * d1 + w2 * d2);
	}
	// normalize and return the result
	if (wsum > 0) result /= wsum;
	gl_FragDepth = result;
}
