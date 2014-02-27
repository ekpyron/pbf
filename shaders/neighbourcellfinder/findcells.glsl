// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

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
