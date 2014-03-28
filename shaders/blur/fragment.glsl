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

layout (binding = 0) uniform sampler2D inputtex;
layout (location = 0) out vec4 color;

// input from vertex shader
in vec2 fTexcoord;

// blur weights
layout (std430, binding = 0) readonly buffer Weights {
	vec2 weights[];
};

// offset scale indicating input size and blur direction
uniform vec2 offsetscale;

void main(void)
{
	vec4 c;
	// sum weighted data
	c = texture (inputtex, fTexcoord) * weights[0].r;
	for (int i = 1; i < weights.length (); i++)
	{
		c += texture (inputtex, fTexcoord + weights[i].g * offsetscale) * weights[i].r;
		c += texture (inputtex, fTexcoord - weights[i].g * offsetscale) * weights[i].r;
	}
	
	// output resulting color
	color = c;
}
