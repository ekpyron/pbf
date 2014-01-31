// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

struct ParticleInfo
{
	vec3 position;
	vec4 oldposition;
};

layout (std430, binding = 0) readonly buffer ParticleBuffer
{
	ParticleInfo particles[];
};

layout (binding = 0, r8i) uniform writeonly iimageBuffer flagtexture;
layout (binding = 1, r32i) uniform writeonly iimage3D gridtexture;

uint GetHash (in vec3 pos)
{
	ivec3 gridpos = ivec3 (clamp (pos, vec3 (0, 0, 0), GRID_SIZE));
	return gridpos.y * GRID_WIDTH * GRID_DEPTH + gridpos.z * GRID_WIDTH + gridpos.x;
}

void main (void)
{
	uint gid;
	gid = gl_GlobalInvocationID.x;
	
	if (gid == 0)
	{
		imageStore (gridtexture, ivec3 (0, 0, 0), ivec4 (0, 0, 0, 0));
		imageStore (flagtexture, 0, ivec4 (0, 0, 0, 0));
		imageStore (flagtexture, int (NUM_PARTICLES), ivec4 (1, 0, 0, 0));
		return;
	}

	ivec3 gridpos = ivec3 (clamp (particles[gid].position, vec3 (0, 0, 0), GRID_SIZE));
	uint hash = gridpos.y * GRID_WIDTH * GRID_DEPTH + gridpos.z * GRID_WIDTH + gridpos.x;
	
	if (hash != GetHash (particles[gid - 1].position))
	{
		imageStore (gridtexture, gridpos, ivec4 (gid, 0, 0, 0));
		imageStore (flagtexture, int (gid), ivec4 (1, 0, 0, 0));
	}
	else
	{
		imageStore (flagtexture, int (gid), ivec4 (0, 0, 0, 0));
	}
}
