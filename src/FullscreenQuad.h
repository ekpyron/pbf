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
	/** Obtain class instance.
	 * FullscreenQuad is a singleton class, so it can't be constructed,
	 * but this function is called to obtain a reference to a global object.
	 * \returns a reference to the global fullscreen quad object
	 *
	 */
	static const FullscreenQuad &Get (void);
	/** Release global object.
	 * FullscreenQuad is a singleton class, so there is a global object that
	 * is created on demand. On application termination this object has to be
	 * released by calling this function.
	 */
	static void Release (void);
    /** Render.
     * Renders the framing.
     */
    void Render (void) const;
private:
    /** Global object.
     * The FullscreenQuad is a singleton class, so there exists only one global instance
     * of this class at any time, which is stored in this static variable.
     *
     */
    static FullscreenQuad *object;
    /** Private constructor.
     * As FullscreenQuad is a singleton class the constructor is private and only used internally
     * to create the global object.
     */
	FullscreenQuad (void);
    /** Destructor.
     * As FullscreenQuad is a singleton class the destructor is private and only used internally
     * to release the global object.
     */
    ~FullscreenQuad (void);
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
