#ifndef SPHERE_H
#define SPHERE_H

#include "common.h"

/** Sphere class.
 * This class represents a three dimensional sphere that can be used for rendering particles.
 */

class Sphere
{
public:
    /** Constructor.
     */
    Sphere (void);
    /** Destructor
     */
    ~Sphere (void);

    /** Specify a position buffer.
     * Specifies a buffer containing the positions for the spheres.
     * \param buffer the buffer object containing the position data
     */
    void SetPositionBuffer (GLuint buffer);

    /** Render the sphere.
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

#endif /* SPHERE_H */
