// simulation/include.glsl is included here

#define BLOCKSIZE 512
#define HALFBLOCKSIZE 256

layout (local_size_x = BLOCKSIZE) in;

struct ParticleInfo
{
	vec4 position;
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
	vec3 pos = data[id].position.xyz;
	ivec3 grid = ivec3 (clamp (pos, vec3 (0, 0, 0), GRID_SIZE));
	return grid.y * GRID_WIDTH * GRID_DEPTH + grid.z * GRID_WIDTH + grid.x;
}

void main (void)
{
	const int gid = int (gl_GlobalInvocationID.x);
	const int lid = int (gl_LocalInvocationIndex);

	uint bits = bitfieldExtract (GetHash (gid), bitshift, 2);
	
	result[blocksum[blocksumoffsets[bits] + gl_WorkGroupID.x] + prefixsum[gid]] = data[gid];
}
