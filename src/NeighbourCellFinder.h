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
	 */
	NeighbourCellFinder (const GLuint &numparticles);
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

    /** Neighbour cell program.
     * Shader program for finding neighbouring cells for a particle.
     */
    ShaderProgram neighbourcells;

    union {
        struct {
            /** Grid texture clear buffer.
             * Buffer used to clear the grid texture;
             */
            GLuint gridclearbuffer;

            /** Neighbour cell buffer.
             * Buffer used to store neighbouring cells for each particle.
             */
            GLuint neighbourcellbuffer;

        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        GLuint buffers[2];
    };

    /** ARB_clear_texture support flag.
     * This flag is set to true if the OpenGL extension ARB_clear_texture is supported
     * by the current OpenGL context and to false, if not.
     *
     */
    bool ARB_clear_texture_supported;

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
