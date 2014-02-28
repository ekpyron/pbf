#include "NeighbourCellFinder.h"

NeighbourCellFinder::NeighbourCellFinder (const GLuint &_numparticles, const glm::ivec3 &_gridsize)
	: numparticles (_numparticles), gridsize (_gridsize)
{
    // determine OpenGL extension support
	ARB_clear_texture_supported = IsExtensionSupported ("GL_ARB_clear_texture");
    bool use_buffer_storage = IsExtensionSupported ("GL_ARB_buffer_storage");

	std::stringstream stream;
	stream << "#version 430 core" << std::endl
		   << "const vec3 GRID_SIZE = vec3 (" << gridsize.x << ", " << gridsize.y << ", " << gridsize.z << ");" << std::endl
		   << "const ivec3 GRID_HASHWEIGHTS = ivec3 (1, " << gridsize.x * gridsize.z <<  ", " << gridsize.z << ");" << std::endl;


	findcells.CompileShader (GL_COMPUTE_SHADER, "shaders/neighbourcellfinder/findcells.glsl", stream.str ());
    findcells.Link ();

    neighbourcells.CompileShader (GL_COMPUTE_SHADER, "shaders/neighbourcellfinder/neighbourcells.glsl", stream.str ());
    neighbourcells.Link ();

	// create buffer objects
	glGenBuffers (2, buffers);

    // allocate grid clear buffer
	// (only needed if GL_ARB_clear_texture is not available)
    if (!ARB_clear_texture_supported)
    {
    	glBindBuffer (GL_PIXEL_UNPACK_BUFFER, gridclearbuffer);
    	glBufferData (GL_PIXEL_UNPACK_BUFFER, sizeof (GLint) * gridsize.x * gridsize.y * gridsize.z, NULL, GL_STATIC_DRAW);
    	{
    		GLint v = -1;
    		glClearBufferData (GL_PIXEL_UNPACK_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &v);
    	}
    }

    // allocate grid texture
    gridtexture.Bind (GL_TEXTURE_3D);
    glTexImage3D (GL_TEXTURE_3D, 0, GL_R32I, gridsize.x, gridsize.y, gridsize.z, 0, GL_RED_INTEGER, GL_INT, NULL);
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
    if (!ARB_clear_texture_supported)
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
    glTexImage3D (GL_TEXTURE_3D, 0, GL_R32I, gridsize.x, gridsize.y, gridsize.z, 0, GL_RED_INTEGER, GL_INT, NULL);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // allocate neighbour cell buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, neighbourcellbuffer);
    if (use_buffer_storage)
        glBufferStorage (GL_SHADER_STORAGE_BUFFER, sizeof (GLuint) * 12 * numparticles, NULL, 0);
    else
    	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLuint) * 12 * numparticles, NULL, GL_DYNAMIC_COPY);

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
	if (ARB_clear_texture_supported)
	{
		GLint v = -1;
		glClearTexImage (gridtexture.get (), 0, GL_RED_INTEGER, GL_INT, &v);
		glMemoryBarrier (GL_TEXTURE_UPDATE_BARRIER_BIT);
	}
	else
	{
		gridtexture.Bind (GL_TEXTURE_3D);
		glBindBuffer (GL_PIXEL_UNPACK_BUFFER, gridclearbuffer);
		glTexSubImage3D (GL_TEXTURE_3D, 0, 0, 0, 0, gridsize.x, gridsize.y, gridsize.z, GL_RED_INTEGER, GL_INT, NULL);
		glBindBuffer (GL_PIXEL_UNPACK_BUFFER, 0);
		glMemoryBarrier (GL_TEXTURE_UPDATE_BARRIER_BIT);
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
