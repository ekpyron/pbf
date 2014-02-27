// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

layout (std430, binding = 0) buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

layout (binding = 0, r32ui) uniform uimageBuffer highlighttexture;

void main (void)
{
	imageAtomicAnd (highlighttexture, int (particlekeys[gl_GlobalInvocationID.x].id), uint (1));
}
