layout (local_size_x = BLOCKSIZE) in;

layout (binding = 0, r32ui) uniform uimageBuffer highlighttexture;

void main (void)
{
	imageAtomicAnd (highlighttexture, int (gl_GlobalInvocationID.x), uint (1));
}
