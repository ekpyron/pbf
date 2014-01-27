// simulation/include.glsl is included here

#define BLOCKSIZE 512
#define HALFBLOCKSIZE 256

layout (local_size_x = BLOCKSIZE) in;

layout (std430, binding = 0) buffer Data
{
	uint data[];
};

layout (std430, binding = 1) readonly buffer BlockSums
{
	uint blocksums[];
};

void main (void)
{
	data[gl_GlobalInvocationID.x] += blocksums[gl_WorkGroupID.x];
}
