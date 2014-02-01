/*
 * SurfaceReconstruction.cpp
 *
 *  Created on: 31.01.2014
 *      Author: daniel
 */

#include "SurfaceReconstruction.h"

SurfaceReconstruction::SurfaceReconstruction (void)
	: offscreen_width (1280), offscreen_height (720)
{
	// load shaders
    particledepthprogram.CompileShader (GL_VERTEX_SHADER, "shaders/particledepth/vertex.glsl");
    particledepthprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/particledepth/fragment.glsl");
    particledepthprogram.Link ();

    depthblurprog.CompileShader (GL_VERTEX_SHADER, "shaders/depthblur/vertex.glsl");
    depthblurprog.CompileShader (GL_FRAGMENT_SHADER, "shaders/depthblur/fragment.glsl");
    depthblurprog.Link ();

    thicknessprog.CompileShader (GL_VERTEX_SHADER, "shaders/thickness/vertex.glsl");
    thicknessprog.CompileShader (GL_FRAGMENT_SHADER, "shaders/thickness/fragment.glsl");
    thicknessprog.Link ();

    fsquadprog.CompileShader (GL_VERTEX_SHADER, "shaders/fsquad/vertex.glsl");
    fsquadprog.CompileShader (GL_FRAGMENT_SHADER, "shaders/fsquad/fragment.glsl");
    fsquadprog.Link ();

    // get uniform location for the depth blur direction
    depthblurdir = depthblurprog.GetUniformLocation ("blurdir");

	// create framebuffer objects
	glGenFramebuffers (5, framebuffers);

	// create texture objects
	glGenTextures (4, textures);

    // create particle depth texture
    glBindTexture (GL_TEXTURE_2D, depthtexture);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, offscreen_width, offscreen_height,
    		0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // create particle depth blur texture
    glBindTexture (GL_TEXTURE_2D, blurtexture);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, offscreen_width, offscreen_height,
    		0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // create particle thickness texture
    glBindTexture (GL_TEXTURE_2D, thicknesstexture);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_R32F, 512, 512, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // create particle thickness blur texture
    glBindTexture (GL_TEXTURE_2D, thicknessblurtexture);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_R32F, 512, 512, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenBuffers (1, &thicknessblurweights);
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, thicknessblurweights);
    Blur::ComputeWeights (GL_SHADER_STORAGE_BUFFER, 10.0f);

    // setup depth framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, depthfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthtexture, 0);

    // setup thickness framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, thicknessfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thicknesstexture, 0);

    // setup thickness blur framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, thicknessblurfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thicknessblurtexture, 0);

    // setup depth horizontal blur framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, depthhblurfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, blurtexture, 0);

    // setup depth vertical blur framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, depthvblurfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthtexture, 0);
    glBindFramebuffer (GL_FRAMEBUFFER, 0);

}

SurfaceReconstruction::~SurfaceReconstruction (void)
{
	// cleanup
	glDeleteTextures (3, textures);
	glDeleteFramebuffers (4, framebuffers);
}

void SurfaceReconstruction::Render (const GLuint &particlebuffer, const GLuint &numparticles,
		const GLuint &width, const GLuint &height)
{
	// get fullscreen quad singleton
	const FullscreenQuad &fullscreenquad = FullscreenQuad::Get ();

	// pass particle buffer to point sprite class
	pointsprite.SetPositionBuffer (particlebuffer, sizeof (particleinfo_t), 0);

	// render point sprites, storing depth
	glBindFramebuffer (GL_FRAMEBUFFER, depthfb);
	glViewport (0, 0, offscreen_width, offscreen_height);
	glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	particledepthprogram.Use ();
	pointsprite.Render (numparticles);

	// use depth texture as input
	glBindFramebuffer (GL_FRAMEBUFFER, depthhblurfb);
	glBindTexture (GL_TEXTURE_2D, depthtexture);
	depthblurprog.Use ();

	glClear (GL_DEPTH_BUFFER_BIT);

	glProgramUniform2f (depthblurprog.get (), depthblurdir, 1.0f / offscreen_width, 0.0f);
	fullscreenquad.Render ();

	glBindFramebuffer (GL_FRAMEBUFFER, depthvblurfb);
	glBindTexture (GL_TEXTURE_2D, blurtexture);

	glClear (GL_DEPTH_BUFFER_BIT);

	glProgramUniform2f (depthblurprog.get (), depthblurdir, 0.0f, 1.0f / offscreen_height);
	fullscreenquad.Render ();

	// render point sprites storing thickness
	glBindFramebuffer (GL_FRAMEBUFFER, thicknessfb);
	{
		float c[] = { 0, 0, 0, 0 };
    	glClearBufferfv (GL_COLOR, 0, c);
	}
	// enable additive blending
	glEnable (GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE);
	glDisable (GL_DEPTH_TEST);
	thicknessprog.Use ();
	glViewport (0, 0, 512, 512);
	pointsprite.Render (numparticles);

	// blur thickness texture
	glDisable (GL_BLEND);
	glBindFramebuffer (GL_FRAMEBUFFER, thicknessblurfb);
	glBindTexture (GL_TEXTURE_2D, thicknesstexture);
	thicknessblur.Apply (glm::vec2 (1.0f / 512.0f, 0), thicknessblurweights);
	glBindFramebuffer (GL_FRAMEBUFFER, thicknessfb);
	glBindTexture (GL_TEXTURE_2D, thicknessblurtexture);
	thicknessblur.Apply (glm::vec2 (0, 1.0f / 512.0f), thicknessblurweights);
	glEnable (GL_BLEND);
	glBindFramebuffer (GL_FRAMEBUFFER, 0);

	glEnable (GL_DEPTH_TEST);

	// use depth texture as input
	glBindTexture (GL_TEXTURE_2D, depthtexture);
	// use thickness texture as input
	glActiveTexture (GL_TEXTURE1);
	glBindTexture (GL_TEXTURE_2D, thicknesstexture);
	glGenerateMipmap (GL_TEXTURE_2D);
	glActiveTexture (GL_TEXTURE0);

	// enable alpha blending
	glBlendFunc (GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

	// render a fullscreen quad
	glViewport (0, 0, width, height);
	fsquadprog.Use ();
	fullscreenquad.Render ();
}
