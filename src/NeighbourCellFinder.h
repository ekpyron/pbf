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
#ifndef NEIGHBOURCELLFINDER_H
#define NEIGHBOURCELLFINDER_H

#include "common.h"
#include "ShaderProgram.h"
#include "RadixSort.h"
#include "Texture.h"

/** Neighbour cell finder class.
 * This class is responsible for finding the grid cell id of the neighbouring
 * particles of each particle.
 *
 */
class NeighbourCellFinder
{
public:
	/** Constructor.
	 * \param numparticles number of particles to process
	 * \param gridsize size of the particle grid
	 */
	NeighbourCellFinder (const GLuint &numparticles, const glm::ivec3 &gridsize);
	/** Destructor.
	 */
	~NeighbourCellFinder (void);

	/** Find neighbour cells.
	 * Finds neighbour cells for the particles in the specified particle buffer.
	 * \param particlebuffer particle buffer to process
	 */
	void FindNeighbourCells (const GLuint &particlebuffer);

	/** Get result.
	 * Returns a buffer texture containing 9 entries for each particle each consisting
	 * of the neighbour cell id and the number of entries in the cell.
	 * \returns the buffer texture containing the found neighbour cells
	 */
	const Texture &GetResult (void) const;
private:
    /** Simulation step shader program.
     * Shader program for the simulation step that finds grid cells in the
     * sorted particle array.
     */
    ShaderProgram findcells;

    /** Grid size.
     * Size of the particle grid.
     */
    const glm::ivec3 gridsize;

    /** Neighbour cell program.
     * Shader program for finding neighbouring cells for a particle.
     */
    ShaderProgram neighbourcells;

    union {
        struct {
            /** Neighbour cell buffer.
             * Buffer used to store neighbouring cells for each particle.
             */
            GLuint neighbourcellbuffer;

        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        GLuint buffers[1];
    };

    /** Grid clear texture.
     * Texture used to clear the grid texture if GL_ARB_clear_texture is not available.
     */
    Texture gridcleartexture;

    /** Grid texture.
     * Texture in which the offset of the first particle for each grid is stored.
     */
    Texture gridtexture;

    /** Grid end texture.
     * Texture in which the offset of the last particle for each grid is stored.
     */
    Texture gridendtexture;

    /** Neighbour cell texture.
     * Texture through which the neighbour cell buffer is accessed.
     */
    Texture neighbourcelltexture;

    /** Number of particles.
     * Stores the number of particles in the simulation.
     */
    const GLuint numparticles;
};

#endif /* NEIGHBOURCELLFINDER_H */
