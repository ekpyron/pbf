#ifndef ICOSAHEDRON_H
#define ICOSAHEDRON_H

#include "common.h"

/** Icosahedron class.
 * This class represents an icosahedron that can be used for rendering particles.
 */

class Icosahedron
{
public:
    /** Constructor.
     */
    Icosahedron (void);
    /** Destructor
     */
    ~Icosahedron (void);

    /** Specify a position buffer.
     * Specifies a buffer containing the positions for the icosahedra.
     * \param buffer the buffer object containing the position data
     */
    void SetPositionBuffer (GLuint buffer, GLsizei stride = 0, GLintptr offset = 0);

    /** Render the icosahedra.
     * \param instances number of instances to render.
     */
    void Render (GLuint instances) const;
private:
    union {
        struct {
            /** Vertex buffer.
             *  OpenGL buffer object storing the vertices of the framing.
             */
            GLuint vertexbuffer;
            /** Index buffer.
             * OpenGL buffer object storing the vertex indices.
             */
            GLuint indexbuffer;
        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        GLuint buffers[2];
    };
    /** Vertex array object.
     * OpenGL vertex array object used to store information about the layout and location
     * of the vertex attributes.
     */
    GLuint vertexarray;
};

#endif /* ICOSAHEDRON_H */
