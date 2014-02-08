/*
 * SurfaceReconstruction.cpp
 *
 *  Created on: 31.01.2014
 *      Author: daniel
 */

#include "SurfaceReconstruction.h"

SurfaceReconstruction::SurfaceReconstruction (void)
	: offscreen_width (1280), offscreen_height (720), envmap (NULL), usenoise (false)
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

    // get uniform location for the depth blur offset scale
    depthbluroffsetscale = depthblurprog.GetUniformLocation ("offsetscale");

	// create framebuffer objects
	glGenFramebuffers (5, framebuffers);

    // create particle depth texture
	depthtexture.Bind (GL_TEXTURE_2D);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, offscreen_width, offscreen_height,
    		0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // create particle depth blur texture
    blurtexture.Bind (GL_TEXTURE_2D);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, offscreen_width, offscreen_height,
    		0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // create particle thickness texture
    thicknesstexture.Bind (GL_TEXTURE_2D);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_R32F, 512, 512, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // create noise texture
    noisetexture.Bind (GL_TEXTURE_2D);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // create particle thickness blur texture
    thicknessblurtexture.Bind (GL_TEXTURE_2D);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_R32F, 512, 512, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenBuffers (2, buffers);
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, thicknessblurweights);
    Blur::ComputeWeights (GL_SHADER_STORAGE_BUFFER, 10.0f);

    glBindBuffer (GL_SHADER_STORAGE_BUFFER, depthblurweights);
    Blur::ComputeWeights (GL_SHADER_STORAGE_BUFFER, 3.0f);

    // setup depth framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, depthfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthtexture.get (), 0);

    // setup thickness framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, thicknessfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thicknesstexture.get (), 0);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, noisetexture.get (), 0);

    // setup thickness blur framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, thicknessblurfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thicknessblurtexture.get (), 0);

    // setup depth horizontal blur framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, depthhblurfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, blurtexture.get (), 0);

    // setup depth vertical blur framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, depthvblurfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthtexture.get (), 0);
    glBindFramebuffer (GL_FRAMEBUFFER, 0);

}

SurfaceReconstruction::~SurfaceReconstruction (void)
{
	// cleanup
	glDeleteFramebuffers (5, framebuffers);
}

void SurfaceReconstruction::EnableNoise (void)
{
	usenoise = true;
	glProgramUniform1i (fsquadprog.get (), fsquadprog.GetUniformLocation ("usenoise"), 1);
	glProgramUniform1i (thicknessprog.get (), thicknessprog.GetUniformLocation ("usenoise"), 1);
}

void SurfaceReconstruction::DisableNoise (void)
{
	usenoise = false;
	glProgramUniform1i (fsquadprog.get (), fsquadprog.GetUniformLocation ("usenoise"), 0);
	glProgramUniform1i (thicknessprog.get (), thicknessprog.GetUniformLocation ("usenoise"), 0);
}

void SurfaceReconstruction::SetEnvironmentMap (const Texture *_envmap)
{
	envmap = _envmap;
	if (envmap != NULL)
	{
		glProgramUniform1i (fsquadprog.get (), fsquadprog.GetUniformLocation ("useenvmap"), 1);
	}
	else
	{
		glProgramUniform1i (fsquadprog.get (), fsquadprog.GetUniformLocation ("useenvmap"), 0);
	}
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

    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, depthblurweights);
    // blur depth
	depthblurprog.Use ();
	for (int i = 0; i < 3; i++)
	{
		depthtexture.Bind (GL_TEXTURE_2D);
		glBindFramebuffer (GL_FRAMEBUFFER, depthhblurfb);
		glClear (GL_DEPTH_BUFFER_BIT);
		glProgramUniform2f (depthblurprog.get (), depthbluroffsetscale, 1.0f / offscreen_width, 0.0f);
		fullscreenquad.Render ();
		glBindFramebuffer (GL_FRAMEBUFFER, depthvblurfb);
		blurtexture.Bind (GL_TEXTURE_2D);
		glClear (GL_DEPTH_BUFFER_BIT);
		glProgramUniform2f (depthblurprog.get (), depthbluroffsetscale, 0.0f, 1.0f / offscreen_height);
		fullscreenquad.Render ();
	}

	// render point sprites storing thickness
	glBindFramebuffer (GL_FRAMEBUFFER, thicknessfb);
	if (usenoise)
	{
		GLuint buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers (2, buffers);
		float c[] = { 0, 0, 0, 0 };
		glClearBufferfv (GL_COLOR, 0, c);
		glClearBufferfv (GL_COLOR, 1, c);
	}
	else
	{
		GLuint buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers (1, buffers);
		float c[] = { 0, 0, 0, 0 };
		glClearBufferfv (GL_COLOR, 0, c);
	}
	// enable additive blending
	glEnable (GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE);
	glDisable (GL_DEPTH_TEST);
	// use depth texture as input
	depthtexture.Bind (GL_TEXTURE_2D);
	thicknessprog.Use ();
	glViewport (0, 0, 512, 512);
	pointsprite.Render (numparticles);

	// blur thickness texture
	glDisable (GL_BLEND);
	glBindFramebuffer (GL_FRAMEBUFFER, thicknessblurfb);
	thicknesstexture.Bind (GL_TEXTURE_2D);
	thicknessblur.Apply (glm::vec2 (1.0f / 512.0f, 0), thicknessblurweights);
	glBindFramebuffer (GL_FRAMEBUFFER, thicknessfb);
	{
		GLuint buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers (1, buffers);
	}
	thicknessblurtexture.Bind (GL_TEXTURE_2D);
	thicknessblur.Apply (glm::vec2 (0, 1.0f / 512.0f), thicknessblurweights);

	// use depth texture as input
	depthtexture.Bind (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
	glEnable (GL_DEPTH_TEST);

	glBindFramebuffer (GL_FRAMEBUFFER, 0);

	// use thickness texture as input
	glActiveTexture (GL_TEXTURE1);
	thicknesstexture.Bind (GL_TEXTURE_2D);
	glGenerateMipmap (GL_TEXTURE_2D);
	// use environment map as input
	if (envmap)
	{
		glActiveTexture (GL_TEXTURE2);
		envmap->Bind (GL_TEXTURE_CUBE_MAP);
	}
	// use noise texture as input
	if (usenoise)
	{
		glActiveTexture (GL_TEXTURE3);
		noisetexture.Bind (GL_TEXTURE_2D);
		glGenerateMipmap (GL_TEXTURE_2D);
	}
	glActiveTexture (GL_TEXTURE0);

	// enable alpha blending
	glBlendFunc (GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

	// render a fullscreen quad
	glViewport (0, 0, width, height);
	fsquadprog.Use ();
	fullscreenquad.Render ();
}
