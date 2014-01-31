#include "Simulation.h"


Simulation::Simulation (void) : width (0), height (0), font ("textures/font.png"),
    last_fps_time (glfwGetTime ()), framecount (0), fps (0), running (false),
    offscreen_width (1280), offscreen_height (720), surfacereconstruction (false),
    sph (GetNumberOfParticles ())
{
    // load shaders
    particleprogram.CompileShader (GL_VERTEX_SHADER, "shaders/particles/vertex.glsl");
    particleprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/particles/fragment.glsl");
    particleprogram.Link ();

    particledepthprogram.CompileShader (GL_VERTEX_SHADER, "shaders/particledepth/vertex.glsl");
    particledepthprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/particledepth/fragment.glsl");
    particledepthprogram.Link ();

    depthblurprog.CompileShader (GL_VERTEX_SHADER, "shaders/depthblur/vertex.glsl");
    depthblurprog.CompileShader (GL_FRAGMENT_SHADER, "shaders/depthblur/fragment.glsl");
    depthblurprog.Link ();

    thicknessprog.CompileShader (GL_VERTEX_SHADER, "shaders/thickness/vertex.glsl");
    thicknessprog.CompileShader (GL_FRAGMENT_SHADER, "shaders/thickness/fragment.glsl");
    thicknessprog.Link ();

    depthblurdir = depthblurprog.GetUniformLocation ("blurdir");

    selectionprogram.CompileShader (GL_VERTEX_SHADER, "shaders/selection/vertex.glsl");
    selectionprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/selection/fragment.glsl");
    selectionprogram.Link ();

    fsquadprog.CompileShader (GL_VERTEX_SHADER, "shaders/fsquad/vertex.glsl");
    fsquadprog.CompileShader (GL_FRAGMENT_SHADER, "shaders/fsquad/fragment.glsl");
    fsquadprog.Link ();

    // create buffer objects
    glGenBuffers (2, buffers);

    // create query objects
    glGenQueries (1, &renderingquery);

    // create texture objects
    glGenTextures (4, textures);

    // create selection depth texture
    glBindTexture (GL_TEXTURE_2D, selectiondepthtexture);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

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
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // create framebuffer objects
    glGenFramebuffers (5, framebuffers);

    // setup selection framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, selectionfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, selectiondepthtexture, 0);

    // setup depth framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, depthfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthtexture, 0);

    // setup thickness framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, thicknessfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thicknesstexture, 0);

    // setup depth horizontal blur framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, depthhblurfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, blurtexture, 0);

    // setup depth vertical blur framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, depthvblurfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthtexture, 0);
    glBindFramebuffer (GL_FRAMEBUFFER, 0);

    // initialize the camera position and rotation and the transformation matrix buffer.
    camera.SetPosition (glm::vec3 (20, 10, 10));
    camera.Rotate (30.0f, 240.0f);
    glBindBufferBase (GL_UNIFORM_BUFFER, 0, transformationbuffer);
    glBufferData (GL_UNIFORM_BUFFER, sizeof (transformationbuffer_t), NULL, GL_DYNAMIC_DRAW);

    // specify lighting parameters
    // and bind the uniform buffer object to binding point 1
    {
        lightparams_t lightparams;
        lightparams.lightpos = glm::vec3 (-20, 80, -2);
        lightparams.spotdir = glm::normalize (glm::vec3 (0.25, -1, 0.25));
        lightparams.spotexponent = 8.0f;
        lightparams.lightintensity = 10000.0f;
        glBindBufferBase (GL_UNIFORM_BUFFER, 1, lightingbuffer);
        glBufferData (GL_UNIFORM_BUFFER, sizeof (lightparams), &lightparams, GL_STATIC_DRAW);
    }

    // enable depth testing
    glEnable (GL_DEPTH_TEST);

    // enable back face culling
    glEnable (GL_CULL_FACE);
    glFrontFace (GL_CCW);

    // set color and depth for the glClear call
    glClearColor (0.25f, 0.25f, 0.25f, 1.0f);
    glClearDepth (1.0f);

    // Initialize particle buffer
    ResetParticleBuffer ();

    // pass the auxiliary buffer as color buffer to the point sprite class
    pointsprite.SetColorBuffer (sph.GetAuxiliaryBuffer (), sizeof (float) * 4, 0);

    // update view matrix
    UpdateViewMatrix ();

    // initialize last frame time
    last_time = glfwGetTime ();
}

Simulation::~Simulation (void)
{
    // cleanup
	glDeleteTextures (4, textures);
	glDeleteFramebuffers (5, framebuffers);
	glDeleteQueries (1, &renderingquery);
    glDeleteBuffers (2, buffers);
}

void Simulation::Resize (const unsigned int &_width, const unsigned int &_height)
{
    // update the stored framebuffer dimensions
    width = _width;
    height = _height;

    // update the stored projection matrix and pass it to the render program
    projmat = glm::perspective (45.0f * float (M_PI / 180.0f), float (width) / float (height), 1.0f, 200.0f);

    glBindBuffer (GL_UNIFORM_BUFFER, transformationbuffer);
    glBufferSubData (GL_UNIFORM_BUFFER, offsetof (transformationbuffer_t, projmat),
    		sizeof (glm::mat4), glm::value_ptr (projmat));
}

void Simulation::UpdateViewMatrix (void)
{
    // update the view matrix
    glBindBuffer (GL_UNIFORM_BUFFER, transformationbuffer);
    glBufferSubData (GL_UNIFORM_BUFFER, offsetof (transformationbuffer_t, viewmat),
    		sizeof (glm::mat4), glm::value_ptr (camera.GetViewMatrix ()));
    glBufferSubData (GL_UNIFORM_BUFFER, offsetof (transformationbuffer_t, invviewmat),
    		sizeof (glm::mat4), glm::value_ptr (glm::inverse (camera.GetViewMatrix ())));

    // update eye position
    glm::vec3 eyepos = camera.GetPosition ();
    glBindBuffer (GL_UNIFORM_BUFFER, lightingbuffer);
    glBufferSubData (GL_UNIFORM_BUFFER, offsetof (lightparams_t, eyepos), sizeof (eyepos), glm::value_ptr (eyepos));
}

void Simulation::OnMouseMove (const double &x, const double &y)
{
    // handle mouse events and pass them to the camera class
    if (glfwGetMouseButton (window, GLFW_MOUSE_BUTTON_LEFT))
    {
        if (glfwGetKey (window, GLFW_KEY_LEFT_CONTROL))
        {
            camera.Zoom (x + y);
        }
        else if (glfwGetKey (window, GLFW_KEY_LEFT_SHIFT))
        {
            camera.MoveX (x);
            camera.MoveY (y);
        }
        else
        {
            camera.Rotate (y, -x);
        }
        UpdateViewMatrix ();
    }
}

void Simulation::OnMouseUp (const int &button)
{
}

void Simulation::OnMouseDown (const int &button)
{
	if (glfwGetKey (window, GLFW_KEY_H))
	{
		// setup viewport according to cursor position
		double xpos, ypos;
		int width, height;
		glfwGetCursorPos (window, &xpos, &ypos);
		glfwGetFramebufferSize (window, &width, &height);
		glBindFramebuffer (GL_FRAMEBUFFER, selectionfb);
        glViewport (-xpos, ypos - height, width, height);

        // clear and fill depth buffer
       	glClear (GL_DEPTH_BUFFER_BIT);
       	selectionprogram.Use ();
       	glProgramUniform1i (selectionprogram.get (), selectionprogram.GetUniformLocation ("write"), 0);
       	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, sph.GetAuxiliaryBuffer ());
       	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, sph.GetParticleBuffer ());
       	pointsprite.Render (GetNumberOfParticles());

       	// only render the closest sphere and write the highlighted flag.
       	glProgramUniform1i (selectionprogram.get (), selectionprogram.GetUniformLocation ("write"), 1);
       	glDepthFunc (GL_EQUAL);
       	pointsprite.Render (GetNumberOfParticles());
       	glDepthFunc (GL_LESS);
		glBindFramebuffer (GL_FRAMEBUFFER, 0);
	}
}

const unsigned int Simulation::GetNumberOfParticles (void) const
{
	// must be a multiple of 512
    return 32 * 32 * 32 * 2;
}

void Simulation::ResetParticleBuffer (void)
{
    // regenerate particle information
    std::vector<particleinfo_t> particles;
    particles.reserve (GetNumberOfParticles ());

    const float scale = 0.94f;

    srand (time (NULL));
    for (int x = 0; x < 32; x++)
    {
        for (int z = 0; z < 32; z++)
        {
            for (int y = 0; y < 32; y++)
            {
                particleinfo_t particle;
                particle.position = glm::vec3 (32.5f, 0.5f, 32.5f) + scale * glm::vec3 (x, y, z);
                particle.position += 0.01f * glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                		float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);
                particle.oldposition = particle.position;
                particle.highlighted = 0;
                particle.vorticity = 0;
                particles.push_back (particle);
            }
        }
    }
    for (int x = 0; x < 32; x++)
    {
        for (int z = 0; z < 32; z++)
        {
            for (int y = 0; y < 32; y++)
            {
                particleinfo_t particle;
                particle.position = glm::vec3 (32.5f + 63.0f, 0.5f, 32.5f + 63.0f) + scale * glm::vec3 (-x, y, -z);
                particle.position += 0.01f * glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                		float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);
                particle.oldposition = particle.position;
                particle.highlighted = 0;
                particle.vorticity = 0;
                particles.push_back (particle);
            }
        }
    }
    //  update the particle buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, sph.GetParticleBuffer ());
    glBufferSubData (GL_SHADER_STORAGE_BUFFER, 0, sizeof (particleinfo_t) * particles.size (), &particles[0]);
    // clear auxiliary buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, sph.GetAuxiliaryBuffer ());
    const float auxdata[] = { 0.25, 0, 1, 1 };
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &auxdata[0]);
}

void Simulation::OnKeyUp (int key)
{
    switch (key)
    {
    // toggle surface reconstruction
    case GLFW_KEY_ENTER:
    	surfacereconstruction = !surfacereconstruction;
    	break;
    // toggle simulation
    case GLFW_KEY_SPACE:
    	running = !running;
    	break;
    // toggle vorticity confinement
    case GLFW_KEY_V:
    	sph.SetVorticityConfinementEnabled (!sph.IsVorticityConfinementEnabled ());
    	break;
    // reset to initial particle configuration
    case GLFW_KEY_TAB:
        ResetParticleBuffer ();
        break;
    // output the queried time frames spent in the
    // different simulation stages
    case GLFW_KEY_T:
    {
    	sph.OutputTiming ();
    	if (glIsQuery (renderingquery))
    	{
    		GLint64 v;
    		glGetQueryObjecti64v (renderingquery, GL_QUERY_RESULT, &v);
    		std::cout << "Rendering: " << double (v) / 1000000.0 << " ms" << std::endl;
    	}
    	break;
    }
    }
}

bool Simulation::Frame (void)
{
    float time_passed = glfwGetTime () - last_time;
    last_time += time_passed;

    GLenum err = glGetError ();
    if (err != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error detected at frame begin: 0x" << std::hex << err << std::endl;
        return false;
    }

    // pass the position buffer to the point sprite class
    pointsprite.SetPositionBuffer (sph.GetParticleBuffer (), sizeof (particleinfo_t), 0);

    // specify the viewport size
    glViewport (0, 0, width, height);

    // clear the color and depth buffer
    glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // render the framing
    framing.Render ();

    // run simulation step 1
    if (running)
    {
    	sph.Run ();
    }

	glBeginQuery (GL_TIME_ELAPSED, renderingquery);
    if (!surfacereconstruction)
    {
    	// render icosahedra/spheres
    	particleprogram.Use ();
    	pointsprite.Render (GetNumberOfParticles ());
    }
    else
    {
    	// render point sprites, storing depth
    	glBindFramebuffer (GL_FRAMEBUFFER, depthfb);
    	glViewport (0, 0, offscreen_width, offscreen_height);
    	glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    	particledepthprogram.Use ();
    	pointsprite.Render (GetNumberOfParticles ());

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
    	pointsprite.Render (GetNumberOfParticles ());
    	glEnable (GL_DEPTH_TEST);

    	glBindFramebuffer (GL_FRAMEBUFFER, 0);

    	// use depth texture as input
    	glBindTexture (GL_TEXTURE_2D, depthtexture);
    	// use thickness texture as input
    	glActiveTexture (GL_TEXTURE1);
    	glBindTexture (GL_TEXTURE_2D, thicknesstexture);
    	glGenerateMipmap (GL_TEXTURE_2D);
    	glActiveTexture (GL_TEXTURE0);

    	// enable alpha blending
    	glBlendFunc (GL_ONE_MINUS_SRC_COLOR, GL_SRC_COLOR);

    	// render a fullscreen quad
    	glViewport (0, 0, width, height);
    	fsquadprog.Use ();
    	fullscreenquad.Render ();

    	glDisable (GL_BLEND);
    }
	glEndQuery (GL_TIME_ELAPSED);


    // determine the framerate every second
    framecount++;
    if (glfwGetTime () >= last_fps_time + 1)
    {
        fps = framecount;
        framecount = 0;
        last_fps_time = glfwGetTime ();
    }
    // display the framerate
    {
        std::stringstream stream;
        stream << "FPS: " << fps << std::endl;
        font.PrintStr (0, 0, stream.str ());
    }
    return true;
}
