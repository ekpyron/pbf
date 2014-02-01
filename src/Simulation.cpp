#include "Simulation.h"


Simulation::Simulation (void) : width (0), height (0), font ("textures/font.png"),
    last_fps_time (glfwGetTime ()), framecount (0), fps (0), running (false),
    usesurfacereconstruction (false), sph (GetNumberOfParticles ())
{
    // load shaders
    particleprogram.CompileShader (GL_VERTEX_SHADER, "shaders/particles/vertex.glsl");
    particleprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/particles/fragment.glsl");
    particleprogram.Link ();

    selectionprogram.CompileShader (GL_VERTEX_SHADER, "shaders/selection/vertex.glsl");
    selectionprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/selection/fragment.glsl");
    selectionprogram.Link ();

    selectiondepthprogram.CompileShader (GL_VERTEX_SHADER, "shaders/selection/vertex.glsl");
    selectiondepthprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/selection/depth.glsl");
    selectiondepthprogram.Link ();

    // create buffer objects
    glGenBuffers (2, buffers);

    // create query objects
    glGenQueries (1, &renderingquery);

    // create texture objects
    glGenTextures (1, textures);

    // create selection depth texture
    glBindTexture (GL_TEXTURE_2D, selectiondepthtexture);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // create framebuffer objects
    glGenFramebuffers (1, framebuffers);

    // setup selection framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, selectionfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, selectiondepthtexture, 0);
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
	glDeleteTextures (1, textures);
	glDeleteFramebuffers (1, framebuffers);
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

        // pass the position buffer to the point sprite class
        pointsprite.SetPositionBuffer (sph.GetParticleBuffer (), sizeof (particleinfo_t), 0);

        // clear and fill depth buffer
       	glClear (GL_DEPTH_BUFFER_BIT);
       	selectiondepthprogram.Use ();
       	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, sph.GetParticleBuffer ());
       	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, sph.GetAuxiliaryBuffer ());
       	pointsprite.Render (GetNumberOfParticles());

       	// only render the closest sphere and write the highlighted flag.
       	selectionprogram.Use ();
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

void Simulation::OnKeyDown (int key)
{
	switch (key)
	{
	// enable external force
	case GLFW_KEY_F:
		sph.SetExternalForce (true);
		break;
	}
}

void Simulation::OnKeyUp (int key)
{
    switch (key)
    {
    // toggle surface reconstruction
    case GLFW_KEY_ENTER:
    	usesurfacereconstruction = !usesurfacereconstruction;
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
    // disable external force
    case GLFW_KEY_F:
    	sph.SetExternalForce (false);
    	break;
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

    // specify the viewport size
    glViewport (0, 0, width, height);

    // clear the color and depth buffer
    glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // render the framing
    framing.Render ();

    // run simulation step 1
    if (running)
    	sph.Run ();

	glBeginQuery (GL_TIME_ELAPSED, renderingquery);
    if (!usesurfacereconstruction)
    {
    	// render icosahedra/spheres
    	particleprogram.Use ();
        // pass the position buffer to the point sprite class
        pointsprite.SetPositionBuffer (sph.GetParticleBuffer (), sizeof (particleinfo_t), 0);
    	pointsprite.Render (GetNumberOfParticles ());
    }
    else
    {
    	// render reconstructed surface
    	surfacereconstruction.Render (sph.GetParticleBuffer (), GetNumberOfParticles (), width, height);
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
