#include "Simulation.h"


Simulation::Simulation (void) : width (0), height (0), font ("textures/font.png"),
    last_fps_time (glfwGetTime ()), framecount (0), fps (0), radixsort (GetNumberOfParticles () >> 9)
{
    // load shaders
    surroundingprogram.CompileShader (GL_VERTEX_SHADER, "shaders/surrounding/vertex.glsl");
    surroundingprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/surrounding/fragment.glsl");
    surroundingprogram.Link ();

    particleprogram.CompileShader (GL_VERTEX_SHADER, "shaders/particles/vertex.glsl");
    particleprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/particles/fragment.glsl");
    particleprogram.Link ();

    predictpos.CompileShader (GL_COMPUTE_SHADER, "shaders/simulation/predictpos.glsl",
    		"shaders/simulation/include.glsl");
    predictpos.Link ();

    findcells.CompileShader (GL_COMPUTE_SHADER, "shaders/simulation/findcells.glsl",
    		"shaders/simulation/include.glsl");
    findcells.Link ();

    calclambda.CompileShader (GL_COMPUTE_SHADER, "shaders/simulation/calclambda.glsl",
    		"shaders/simulation/include.glsl");
    calclambda.Link ();

    updatepos.CompileShader (GL_COMPUTE_SHADER, "shaders/simulation/updatepos.glsl",
    		"shaders/simulation/include.glsl");
    updatepos.Link ();

    // create buffer objects
    glGenBuffers (6, buffers);

    // initialize the camera position and rotation and the transformation matrix buffer.
    camera.SetPosition (glm::vec3 (20, 10, 10));
    camera.Rotate (30.0f, 240.0f);
    glBindBufferBase (GL_UNIFORM_BUFFER, 0, transformationbuffer);
    glBufferData (GL_UNIFORM_BUFFER, sizeof (glm::mat4), glm::value_ptr (projmat * camera.GetViewMatrix()),
            GL_DYNAMIC_DRAW);


    // specify lighting parameters
    // and bind the uniform buffer object to binding point 1
    {
        struct lightparams {
            glm::vec3 lightpos;
            float padding;
            glm::vec3 spotdir;
            float spotexponent;
            float lightintensity;
        } lightparams;
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

    // allocate lambda buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, lambdabuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * GetNumberOfParticles (), NULL, GL_DYNAMIC_DRAW);

    // allocate grid buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, gridbuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLuint) * 128 * 64 * 128, NULL, GL_DYNAMIC_DRAW);

    // allocate flag buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, flagbuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLbyte) * (GetNumberOfParticles () + 1), NULL, GL_DYNAMIC_DRAW);

    // create flag texture
    glGenTextures (1, &flagtexture);
    glBindTexture (GL_TEXTURE_BUFFER, flagtexture);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_R8I, flagbuffer);

    // allocate auxillary buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, auxbuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * 4 * GetNumberOfParticles (), NULL, GL_DYNAMIC_DRAW);
    // clear auxiliary buffer
    const float auxdata[] = { 0.25, 0, 1, 1 };
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &auxdata[0]);

    // pass the auxiliary buffer as color buffer to the icosahedron class
    icosahedron.SetColorBuffer (auxbuffer, sizeof (float) * 4, 0);

    // initialize last frame time
    last_time = glfwGetTime ();
}

Simulation::~Simulation (void)
{
    // cleanup
    glDeleteBuffers (6, buffers);
}

void Simulation::Resize (const unsigned int &_width, const unsigned int &_height)
{
    // update the stored framebuffer dimensions
    width = _width;
    height = _height;

    // update the stored projection matrix and pass it to the render program
    projmat = glm::perspective (45.0f * float (M_PI / 180.0f), float (width) / float (height), 0.1f, 1000.0f);
    glBindBuffer (GL_UNIFORM_BUFFER, transformationbuffer);
    glBufferSubData (GL_UNIFORM_BUFFER, 0, sizeof (glm::mat4), glm::value_ptr (projmat * camera.GetViewMatrix ()));
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

        // update the view matrix
        glBindBuffer (GL_UNIFORM_BUFFER, transformationbuffer);
        glBufferSubData (GL_UNIFORM_BUFFER, 0, sizeof (glm::mat4),
                glm::value_ptr (projmat * camera.GetViewMatrix ()));
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
    srand (time (NULL));
    for (int x = 0; x < 32; x++)
    {
        for (int z = 0; z < 32; z++)
        {
            for (int y = 0; y < 32; y++)
            {
                particleinfo_t particle;
                particle.position = glm::vec3 (32.5 + x, y + 0.5, 32.5 + z);
                particle.position += 0.01f * glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                		float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);
                particle.oldposition = particle.position;
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
                particle.position = glm::vec3 (32.5 + 63 - x, y + 0.5, 32.5 + 63 - z);
                particle.position += 0.01f * glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                		float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);
                particle.oldposition = particle.position;
                particles.push_back (particle);
            }
        }
    }
    //  update the particle buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, radixsort.GetBuffer ());
    glBufferSubData (GL_SHADER_STORAGE_BUFFER, 0, sizeof (particleinfo_t) * particles.size (), &particles[0]);
    // clear auxiliary buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, auxbuffer);
    const float auxdata[] = { 0.25, 0, 1, 1 };
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &auxdata[0]);
}

void Simulation::OnKeyUp (int key)
{
    switch (key)
    {
    case GLFW_KEY_TAB:
        ResetParticleBuffer ();
        break;
    case GLFW_KEY_D:
    {
    	glBindBuffer (GL_SHADER_STORAGE_BUFFER, flagbuffer);
    	GLuint *data = reinterpret_cast<GLuint*> (glMapBuffer (GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));
    	for (int i = 0; i < GetNumberOfParticles(); i++)
    	{
    		std::cout << data[i] << " ";
    	}
    	std::cout << std::endl;
    	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
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

    // pass the position buffer to the icosahedron class
    icosahedron.SetPositionBuffer (radixsort.GetBuffer (), sizeof (particleinfo_t), 0);

    // specify the viewport size
    glViewport (0, 0, width, height);

    // clear the color and depth buffer
    glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // render the framing
    surroundingprogram.Use ();
    framing.Render ();

    // run simulation step 1
    if (glfwGetKey (window, GLFW_KEY_SPACE))
    {
        // clear grid buffer
        glBindBuffer (GL_SHADER_STORAGE_BUFFER, gridbuffer);
        {
        	GLint v = -1;
        	glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &v);
        }

        // clear lambda buffer
        glBindBuffer (GL_SHADER_STORAGE_BUFFER, lambdabuffer);
        glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, NULL);

        // clear auxiliary buffer
        glBindBuffer (GL_SHADER_STORAGE_BUFFER, auxbuffer);
        const float auxdata[] = { 0.25, 0, 1, 1 };
        glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &auxdata[0]);

        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, radixsort.GetBuffer ());
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, auxbuffer);

        predictpos.Use ();
        glMemoryBarrier (GL_BUFFER_UPDATE_BARRIER_BIT);
        glDispatchCompute (GetNumberOfParticles () >> 8, 1, 1);
        glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);

        // sort particles
        radixsort.Run (21);

        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, radixsort.GetBuffer ());
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, lambdabuffer);
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, auxbuffer);
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, gridbuffer);

        glBindImageTexture (0, flagtexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8I);

        findcells.Use ();
        glMemoryBarrier (GL_BUFFER_UPDATE_BARRIER_BIT);
        glDispatchCompute (GetNumberOfParticles () >> 8, 1, 1);
        glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);

        for (auto i = 0; i < 3; i++) {
        calclambda.Use ();
        glDispatchCompute (GetNumberOfParticles () >> 8, 1, 1);
        glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);

        updatepos.Use ();
        glDispatchCompute (GetNumberOfParticles () >> 8, 1, 1);
        glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }

    // render spheres
    particleprogram.Use ();
    icosahedron.Render (GetNumberOfParticles ());

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
