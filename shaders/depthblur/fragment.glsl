#version 430 core

// output color
layout (location = 0) out float result;

// input from vertex shader
in vec2 fTexcoord;

layout (binding = 0) uniform sampler2D depthtex;

// blur directioon
uniform vec2 blurdir;
//uniform float radius;
//uniform float scale;
//uniform float falloff;
const float radius = 24.0f;
const float scale = 1.0f / 16.0f;
const float falloff = 1000.0f;

void main (void)
{
	// fetch depth from texture
	float depth = texture (depthtex, fTexcoord, 0).x;
	if (depth == 1)
		discard;

	float sum = 0;
	float wsum = 0;
	for (float x = -radius; x<= radius; x += 1.0)
	{
		float d = texture (depthtex, fTexcoord + x * blurdir).x;
		if (d == 1.0)
			continue;
		
		float r = x * scale;
		float w = exp (-r * r);
		
		float r2 = (d - depth) * falloff;
		float g = exp (-r2 * r2);
		
		sum += d * w * g;
		wsum += w * g;
	}
	if (wsum > 0) sum /= wsum;
	
	gl_FragDepth = sum;
}
