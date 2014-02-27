// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

layout (std430, binding = 0) buffer ParticleKeys
{
	vec4 particlekeys[];
};

layout (binding = 2) uniform isamplerBuffer neighbourcelltexture;
layout (binding = 0, r32ui) uniform uimageBuffer highlighttexture;

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
	int id = floatBitsToInt (particlekeys[gl_GlobalInvocationID.x].w);
	
	uint flag = imageLoad (highlighttexture, id).x;
	
	if ((flag & 1) == 1)
	{
		FOR_EACH_NEIGHBOUR(j)
		{
			imageAtomicOr (highlighttexture, floatBitsToInt (particlekeys[j].w), uint(2));
		}
		END_FOR_EACH_NEIGHBOUR(j)
	}	
}
