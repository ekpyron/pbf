#ifndef SURFACERECONSTRUCTION_H
#define SURFACERECONSTRUCTION_H

#include "common.h"
#include "PointSprite.h"
#include "ShaderProgram.h"
#include "FullscreenQuad.h"
#include "Blur.h"

class SurfaceReconstruction
{
public:
	SurfaceReconstruction (void);
	~SurfaceReconstruction (void);
	void Render (const GLuint &particlebuffer, const GLuint &numparticles, const GLuint &width, const GLuint &height);
private:

    /** Particle depth shader program.
     * Shader program for rendering the particle depths only.
    */
    ShaderProgram particledepthprogram;

    /** Fullscreen quad shader program.
     * Shader program for rendering the fullscreen quad.
     */
    ShaderProgram fsquadprog;

    /** Depth blur shader program.
     * Shader program for blurring the particle depth texture.
     */
    ShaderProgram depthblurprog;

    /** Thickness program.
     * Shader program for storing the particle thickness.
     */
    ShaderProgram thicknessprog;

    /** Depth blur direction uniform location.
     * Uniform location for the uniform variable storing the direction of the depth blur.
     */
    GLuint depthblurdir;

    /** Point Sprite.
     * Takes care of rendering particles as point sprites.
     */
    PointSprite pointsprite;

    /** Thickness blur.
     * Takes care of blurring the thickness image.
     */
    Blur thicknessblur;

	/** Thickness blur weights.
	 * Weights used for blurring the thickness texture.
	 */
	GLuint thicknessblurweights;

    union {
    	struct {
    		/** Depth framebuffer.
    		 * Framebuffer object used to store the particle depths.
    		 */
    		GLuint depthfb;
    		/** Horizontal depth blur framebuffer.
    		 * Framebuffer object used for blurring the particle depths horizontally.
    		 */
    		GLuint depthhblurfb;
    		/** Vertical depth blur framebuffer.
    		 * Framebuffer object used for blurring the particle depths vertically.
    		 */
    		GLuint depthvblurfb;
    		/** Thickness framebuffer.
    		 * Framebuffer object used for rendering to the thickness texture.
    		 */
    		GLuint thicknessfb;
    		/** Thickness blur framebuffer.
    		 * Framebuffer object used for blurring the thickness texture.
    		 */
    		GLuint thicknessblurfb;
    	};
    	/** Framebuffer objects.
    	 * The framebuffer objects are stored in a union, so that it is possible
    	 * to create/delete all texture objects with a single OpenGL call.
    	 */
    	GLuint framebuffers[5];
    };

    union {
    	struct {
    		/** Particle depth texture.
    		 * Texture to store the particle depths.
    		 */
    		GLuint depthtexture;

    		/** Temporary blur texture.
    		 * Texture to store temporary steps during blurring.
    		 */
    		GLuint blurtexture;
            /** Thickness texture.
             * Texture in which the water thickness is stored.
             */
            GLuint thicknesstexture;
            /** Thickness blur texture.
             * Temporary texture used for blurring the water thickness.
             */
            GLuint thicknessblurtexture;
    	};
    	/** Texture objects.
    	 * The texture objects are stored in a union, so that it is possible
    	 * to create/delete all texture objects with a single OpenGL call.
    	 */
    	GLuint textures[4];
    };

    /** Offscreen framebuffer width.
     * Width of the offscreen framebuffer.
     */
    unsigned int offscreen_width;
    /** Offscreen framebuffer height.
     * Height of the offscreen framebuffer.
     */
    unsigned int offscreen_height;

};

#endif /* SURFACERECONSTRUCTION_H */
