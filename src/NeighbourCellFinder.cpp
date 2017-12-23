/*
 * Copyright (c) 2013-2014 Daniel Kirchner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE ANDNONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "NeighbourCellFinder.h"

NeighbourCellFinder::NeighbourCellFinder (const GLuint &_numparticles, const glm::ivec3 &_gridsize)
	: numparticles (_numparticles), gridsize (_gridsize)
{
	std::stringstream stream;
	stream << "const vec3 GRID_SIZE = vec3 (" << gridsize.x << ", " << gridsize.y << ", " << gridsize.z << ");" << std::endl
		   << "const ivec3 GRID_HASHWEIGHTS = ivec3 (1, " << gridsize.x * gridsize.z <<  ", " << gridsize.x << ");" << std::endl
		   << "#define BLOCKSIZE 256" << std::endl;


	findcells.CompileShader (GL_COMPUTE_SHADER, "shaders/neighbourcellfinder/findcells.glsl", stream.str ());
    findcells.Link ();

    neighbourcells.CompileShader (GL_COMPUTE_SHADER, "shaders/neighbourcellfinder/neighbourcells.glsl", stream.str ());
    neighbourcells.Link ();

	// create buffer objects
	glGenBuffers (1, buffers);

    // allocate grid clear buffer
	// (only needed if GL_ARB_clear_texture is not available)
	GLuint tmpbuffer;
    if (!GLEXTS.ARB_clear_texture)
    {
    	glGenBuffers (1, &tmpbuffer);
    	glBindBuffer (GL_PIXEL_UNPACK_BUFFER, tmpbuffer);
    	glBufferData (GL_PIXEL_UNPACK_BUFFER, sizeof (GLint) * gridsize.x * gridsize.y * gridsize.z, NULL, GL_STATIC_DRAW);
    	{
    		GLint v = -1;
    		glClearBufferData (GL_PIXEL_UNPACK_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &v);
    	}
    	gridcleartexture.Bind (GL_TEXTURE_3D);
        glTexImage3D (GL_TEXTURE_3D, 0, GL_R32I, gridsize.x, gridsize.y, gridsize.z, 0, GL_RED_INTEGER, GL_INT, NULL);
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
    if (!GLEXTS.ARB_clear_texture)
    {
    	glBindBuffer (GL_PIXEL_UNPACK_BUFFER, 0);
    	glDeleteBuffers (1, &tmpbuffer);
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
   	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLuint) * 12 * numparticles, NULL, GL_DYNAMIC_COPY);

    // create neighbour cell texture
    neighbourcelltexture.Bind (GL_TEXTURE_BUFFER);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_RGBA32I, neighbourcellbuffer);

}

NeighbourCellFinder::~NeighbourCellFinder (void)
{
	// cleanup
	glDeleteBuffers (1, buffers);
}

const Texture &NeighbourCellFinder::GetResult (void) const
{
	return neighbourcelltexture;
}

void NeighbourCellFinder::FindNeighbourCells (const GLuint &particlebuffer)
{
    // clear grid buffer
	if (GLEXTS.ARB_clear_texture)
	{
		GLint v = -1;
		glClearTexImage (gridtexture.get (), 0, GL_RED_INTEGER, GL_INT, &v);
	}
	else
	{
		glCopyImageSubData (gridcleartexture.get (), GL_TEXTURE_3D, 0, 0, 0, 0,
				gridtexture.get (), GL_TEXTURE_3D, 0, 0, 0, 0,
				gridsize.x, gridsize.y, gridsize.z);
	}

    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, particlebuffer);

    // find grid cells: this will set up grid textures containing boundaries of grid cells with begin and end:
    // - gridtexture(i,j,k) = n -> particle n is the first particle of the (i,j,k) box
    // - gridendtexture(i,j,k) = m -> particle m-1 is the last particle of the (i,j,k) box
    // - gridtexture(i,j,k) = -1 -> no particle in this box
    glBindImageTexture (0, gridtexture.get (), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32I);
    glBindImageTexture (1, gridendtexture.get (), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32I);
    findcells.Use ();
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
