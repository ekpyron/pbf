// simulation/include.glsl is included here

#define BLOCKSIZE 512
#define HALFBLOCKSIZE 256

layout (local_size_x = BLOCKSIZE) in;

struct ParticleInfo
{
	vec3 position;
	vec3 oldposition;
};

layout (std430, binding = 0) readonly buffer Data
{
	ParticleInfo data[];
};

layout (std430, binding = 1) readonly buffer PrefixSum
{
	uint prefixsum[];
};

layout (std430, binding = 2) readonly buffer BlockSum
{
	uint blocksum[];
};

layout (std430, binding = 3) writeonly buffer Result
{
	ParticleInfo result[];
};

uniform uvec4 blocksumoffsets;

uniform int bitshift;

uint GetHash (int id)
{
	vec3 pos = data[id].position;
	
	ivec3 grid;
	grid.x = clamp (int (floor (pos.x)), 0, GRID_WIDTH);
	grid.y = clamp (int (floor (pos.y)), 0, GRID_HEIGHT);
	grid.z = clamp (int (floor (pos.z)), 0, GRID_DEPTH);
	
	return grid.y * GRID_WIDTH * GRID_DEPTH + grid.z * GRID_WIDTH + grid.x;
}

void main (void)
{
	const int gid = int (gl_GlobalInvocationID.x);
	const int lid = int (gl_LocalInvocationIndex);

	uint bits = (GetHash (gid) & (3 << bitshift)) >> bitshift;
	
	result[blocksum[blocksumoffsets[bits] + gl_WorkGroupID.x] + prefixsum[gid]] = data[gid];
}
