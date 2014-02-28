// header is included here

layout (local_size_x = BLOCKSIZE) in;

layout (std430, binding = 0) readonly buffer Data
{
	vec4 data[];
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
	vec4 result[];
};

uniform uvec4 blocksumoffsets;

uniform int bitshift;

uint GetHash (int id)
{
	vec3 pos = data[id].xyz;
	ivec3 grid = ivec3 (clamp (pos, vec3 (0, 0, 0), GRID_SIZE));
	return uint (dot (grid, GRID_HASHWEIGHTS));
}

void main (void)
{
	const int gid = int (gl_GlobalInvocationID.x);
	const int lid = int (gl_LocalInvocationIndex);

	uint bits = bitfieldExtract (GetHash (gid), bitshift, 2);
	
	result[blocksum[blocksumoffsets[bits] + gl_WorkGroupID.x] + prefixsum[gid]] = data[gid];
}
