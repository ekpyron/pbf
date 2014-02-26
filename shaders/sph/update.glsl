// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

layout (std430, binding = 0) buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

layout (binding = 0, rgba32f) uniform imageBuffer positiontexture;
layout (binding = 1, rgba32f) uniform imageBuffer velocitytexture;

layout (binding = 2) uniform isamplerBuffer neighbourcelltexture;

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
	ParticleKey key = particlekeys[gl_GlobalInvocationID.x];
	uint particleid = key.id;
	vec3 position = key.position;
	
	vec3 oldposition = imageLoad (positiontexture, int (particleid)).xyz;

	// calculate velocity	
	vec3 velocity = (position - oldposition) / timestep;
	
	// update position and velocity
	imageStore (positiontexture, int (particleid), vec4 (position, 0));
	imageStore (velocitytexture, int (particleid), vec4 (velocity, 0));
}
