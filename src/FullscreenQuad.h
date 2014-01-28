#ifndef FULLSCREENQUAD_H
#define FULLSCREENQUAD_H

#include "common.h"
#include "Texture.h"

/** Fullscreen quad.
 * This class renders a full screen quad.
 */
class FullscreenQuad
{
public:
    /** Constructor.
     */
	FullscreenQuad (void);
    /** Destructor.
     */
    ~FullscreenQuad (void);
    /** Render.
     * Renders the framing.
     */
    void Render (void);
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

#endif /* FULLSCREENQUAD_H */
