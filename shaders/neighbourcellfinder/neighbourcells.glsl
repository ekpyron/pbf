// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

layout (std430, binding = 0) readonly buffer ParticleKeys
{
	vec4 particlekeys[];
};

layout (binding = 0) uniform isampler3D gridtexture;
layout (binding = 1) uniform isampler3D gridendtexture;

layout (binding = 0, rgba32i) uniform writeonly iimageBuffer neighbourtexture;

// neighbour grids in y and z direction
const ivec3 gridoffsets[9] = {
	ivec3 (0, -1, -1),
	ivec3 (0, -1, 0),
	ivec3 (0, -1, 1),
	ivec3 (0, 0, -1),
	ivec3 (0, 0, 0),
	ivec3 (0, 0, 1),
	ivec3 (0, 1, -1),
	ivec3 (0, 1, 0),
	ivec3 (0, 1, 1)
};

// offset between grids in x direction
const ivec3 gridxoffset = ivec3 (1, 0, 0);

void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.x;

	ivec3 gridpos = ivec3 (particlekeys[particleid].xyz);
	
	int cells[9];

	// go through all 9 neighbour directions in y/z direction 
	for (int o = 0; o < 9; o++) {
		int numcells = 0;
		int entries = 0;
		int cell = -1;
		
		// got through all cells in x direction
		for (int j = -1; j <= 1; j++)
		{
			// fetch its starting position
			int c = texelFetch (gridtexture, gridpos + gridoffsets[o] + j * gridxoffset, 0).x;
			// store the position, if we don't already have a starting point
			if (cell == -1) cell = c;
			// if the cell exists
			if (c != -1)
			{
				// lookup its size and update entry count
				int end = texelFetch (gridendtexture, gridpos + gridoffsets[o] + j * gridxoffset, 0).x;
				entries += end - c;
			}
		}
		
		cells[o] = cell + (entries<<24);
	}

	for (int i = 0; i < 3; i++)
	{
		// store everything in the neighbour texture
		imageStore (neighbourtexture, int (particleid * 3 + i), ivec4 (cells[i*3+0], cells[i*3+1], cells[i*3+2], 0));
	}

}
