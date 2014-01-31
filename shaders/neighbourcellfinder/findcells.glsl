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

	ivec3 gridpos = ivec3 (clamp (particles[gid].position, vec3 (0, 0, 0), GRID_SIZE));
	uint hash = gridpos.y * GRID_WIDTH * GRID_DEPTH + gridpos.z * GRID_WIDTH + gridpos.x;
	
	ivec3 gridpos2 = ivec3 (clamp (particles[gid - 1].position, vec3 (0, 0, 0), GRID_SIZE));
	uint hash2 = gridpos2.y * GRID_WIDTH * GRID_DEPTH + gridpos2.z * GRID_WIDTH + gridpos2.x;
	
	if (hash != hash2)
	{
		imageStore (gridtexture, gridpos, ivec4 (gid, 0, 0, 0));
		imageStore (gridendtexture, gridpos2, ivec4 (gid, 0, 0, 0));
	}
}
