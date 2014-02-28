// header is included here

layout (local_size_x = 256) in;

layout (std430, binding = 0) buffer ParticleKeys
{
	vec4 particlekeys[];
};

layout (binding = 0, r32ui) uniform uimageBuffer highlighttexture;

void main (void)
{
	imageAtomicAnd (highlighttexture, floatBitsToInt (particlekeys[gl_GlobalInvocationID.x].w), uint (1));
}
