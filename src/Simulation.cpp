#include "Simulation.h"


Simulation::Simulation (void) : width (0), height (0), font ("textures/font.png"),
    last_fps_time (glfwGetTime ()), framecount (0), fps (0)
{
    // load shaders
    surroundingprogram.CompileShader (GL_VERTEX_SHADER, "shaders/surrounding/vertex.glsl");
    surroundingprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/surrounding/fragment.glsl");
    surroundingprogram.Link ();

    particleprogram.CompileShader (GL_VERTEX_SHADER, "shaders/particles/vertex.glsl");
    particleprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/particles/fragment.glsl");
    particleprogram.Link ();

    // create buffer objects
    glGenBuffers (3, buffers);

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

    // enable multisampling
    glEnable (GL_MULTISAMPLE);

    // enable depth testing
    glEnable (GL_DEPTH_TEST);

    // enable back face culling
    glEnable (GL_CULL_FACE);
    glFrontFace (GL_CCW);

    // set color and depth for the glClear call
    glClearColor (0.25f, 0.25f, 0.25f, 1.0f);
    glClearDepth (1.0f);

    // populate the position buffer
    std::vector<glm::vec3> positions;
    for (int x = -64; x < 64; x++)
    {
        for (int z = -32; z < 32; z++)
        {
            for (int y = 0; y < 8; y++)
            {
                positions.push_back (0.2f * glm::vec3 (x, y + 1, z));
            }
        }
    }
    glBindBuffer (GL_ARRAY_BUFFER, positionbuffer);
    glBufferData (GL_ARRAY_BUFFER, sizeof (glm::vec3) * positions.size (), &positions[0], GL_DYNAMIC_DRAW);

    // pass the position buffer to the sphere and icosahedron class
    icosahedron.SetPositionBuffer (positionbuffer);
}

Simulation::~Simulation (void)
{
    // cleanup
    glDeleteBuffers (3, buffers);
}

void Simulation::Resize (const unsigned int &_width, const unsigned int &_height)
{
    // update the stored framebuffer dimensions
    width = _width;
    height = _height;

    // update the stored projection matrix and pass it to the render program
    projmat = glm::perspective (45.0f, float (width) / float (height), 0.1f, 1000.0f);
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

void Simulation::Frame (void)
{
    // specify the viewport size
    glViewport (0, 0, width, height);

    // clear the color and depth buffer
    glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // render the framing
    surroundingprogram.Use ();
    framing.Render ();

    // render spheres
    particleprogram.Use ();
    icosahedron.Render (128 * 64 * 8);

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
}
