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
#version 430 core

// enable early z culling
layout (early_fragment_tests) in;

// output color
layout (location = 0) out vec4 color;

// input from vertex shader
in vec3 fPosition;
in vec3 fNormal;

// lighting parameters
layout (binding = 1, std140) uniform LightingBuffer
{
	vec3 lightpos;
	vec3 spotdir;
	vec3 eyepos;
	float spotexponent;
	float lightintensity;
};

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 viewmat;
	mat4 projmat;
};

void main (void)
{
	// lighting calculations
	// obtain light direction and distance
	vec3 lightdir = lightpos - fPosition;
	float lightdist = length (lightdir);
	lightdir /= lightdist;

	// compute light intensity as the cosine of the angle
	// between light direction and normal direction
	float intensity = max (dot (lightdir, fNormal), 0);

	// apply distance attenuation and light intensity
	intensity /= lightdist * lightdist;
	intensity *= lightintensity;

	// spot light effect
	float angle = dot (spotdir, - lightdir);
	intensity *= pow (angle, spotexponent);

	// ambient light
	intensity += 0.1;

	// fetch texture value and output resulting color
	color = clamp (intensity, 0, 1) * vec4 (1, 1, 1, 1);
}
