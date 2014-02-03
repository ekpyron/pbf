#include "NeighbourCellFinder.h"

NeighbourCellFinder::NeighbourCellFinder (const GLuint &_numparticles) : numparticles (_numparticles)
{
    findcells.CompileShader (GL_COMPUTE_SHADER, "shaders/neighbourcellfinder/findcells.glsl",
    		"shaders/simulation/include.glsl");
    findcells.Link ();

    neighbourcells.CompileShader (GL_COMPUTE_SHADER, "shaders/neighbourcellfinder/neighbourcells.glsl",
    		"shaders/simulation/include.glsl");
    neighbourcells.Link ();

	// create buffer objects
	glGenBuffers (2, buffers);

    // allocate grid clear buffer
	// (only needed if GL_ARB_clear_texture is not available)
    if (!GL_ARB_clear_texture)
    {
    	glBindBuffer (GL_PIXEL_UNPACK_BUFFER, gridclearbuffer);
    	glBufferData (GL_PIXEL_UNPACK_BUFFER, sizeof (GLint) * 128 * 64 * 128, NULL, GL_DYNAMIC_DRAW);
    	{
    		GLint v = -1;
    		glClearBufferData (GL_PIXEL_UNPACK_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &v);
    	}
    }

    // allocate grid texture
    gridtexture.Bind (GL_TEXTURE_3D);
    glTexImage3D (GL_TEXTURE_3D, 0, GL_R32I, 128, 64, 128, 0, GL_RED_INTEGER, GL_INT, NULL);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    {
    	GLint border[] = { -1, -1, -1, -1 };
    	glTexParameterIiv (GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, border);
    }

    // clear grid texture
    if (!GL_ARB_clear_texture)
    {
    	glBindBuffer (GL_PIXEL_UNPACK_BUFFER, 0);
    }
    else
    {
    	GLint v = -1;
    	glClearTexImage (gridtexture.get (), 0, GL_RED_INTEGER, GL_INT, &v);
    }

    // allocate grid end texture
    gridendtexture.Bind (GL_TEXTURE_3D);
    glTexImage3D (GL_TEXTURE_3D, 0, GL_R32I, 128, 64, 128, 0, GL_RED_INTEGER, GL_INT, NULL);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // allocate neighbour cell buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, neighbourcellbuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLuint) * 12 * numparticles, NULL, GL_DYNAMIC_DRAW);

    // create neighbour cell texture
    neighbourcelltexture.Bind (GL_TEXTURE_BUFFER);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_RGBA32I, neighbourcellbuffer);

}

NeighbourCellFinder::~NeighbourCellFinder (void)
{
	// cleanup
	glDeleteBuffers (2, buffers);
}

const Texture &NeighbourCellFinder::GetResult (void) const
{
	return neighbourcelltexture;
}

void NeighbourCellFinder::FindNeighbourCells (const GLuint &particlebuffer)
{
    // clear grid buffer
	if (GL_ARB_clear_texture)
	{
		GLint v = -1;
		glClearTexImage (gridtexture.get (), 0, GL_RED_INTEGER, GL_INT, &v);
	}
	else
	{
		gridtexture.Bind (GL_TEXTURE_3D);
		glBindBuffer (GL_PIXEL_UNPACK_BUFFER, gridclearbuffer);
		glTexSubImage3D (GL_TEXTURE_3D, 0, 0, 0, 0, 128, 64, 128, GL_RED_INTEGER, GL_INT, NULL);
		glBindBuffer (GL_PIXEL_UNPACK_BUFFER, 0);
	}

    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, particlebuffer);

    // find grid cells
    glBindImageTexture (0, gridtexture.get (), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32I);
    glBindImageTexture (1, gridendtexture.get (), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32I);
    findcells.Use ();
    glMemoryBarrier (GL_BUFFER_UPDATE_BARRIER_BIT);
    glDispatchCompute (numparticles >> 8, 1, 1);
    glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // grid and flag textures as input
    gridtexture.Bind (GL_TEXTURE_3D);
    glActiveTexture (GL_TEXTURE1);
    gridendtexture.Bind (GL_TEXTURE_3D);
    glActiveTexture (GL_TEXTURE0);

    // find neighbour cells for each particle
    glBindImageTexture (0, neighbourcelltexture.get (), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32I);
    neighbourcells.Use ();
    glDispatchCompute (numparticles >> 8, 1, 1);
    glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

}
