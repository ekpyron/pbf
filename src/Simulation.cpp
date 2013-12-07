#include "Simulation.h"


Simulation::Simulation (void) : width (0), height (0), font ("textures/font.png"),
    last_fps_time (glfwGetTime ()), framecount (0), fps (0)
{
    // load shaders
    renderprogram.CompileShader (GL_VERTEX_SHADER, "shaders/vertex.glsl");
    renderprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/fragment.glsl");
    renderprogram.Link ();

    // get uniform locations
    projmat_location = renderprogram.GetUniformLocation ("projmat");
    viewmat_location = renderprogram.GetUniformLocation ("viewmat");

    // initialize the view matrix as the identity matrix
    glProgramUniformMatrix4fv (renderprogram.get (), viewmat_location, 1, GL_FALSE, glm::value_ptr (glm::mat4 (1)));

    // specify lighting parameters
    glProgramUniform3f (renderprogram.get (), renderprogram.GetUniformLocation ("lightpos"), 0, 80, 0);
    glProgramUniform3f (renderprogram.get (), renderprogram.GetUniformLocation ("spotdir"), 0, -1, 0);
    glProgramUniform1f (renderprogram.get (), renderprogram.GetUniformLocation ("spotexponent"), 8.0f);
    glProgramUniform1f (renderprogram.get (), renderprogram.GetUniformLocation ("lightintensity"), 10000.0f);

    // enable multisampling
    glEnable (GL_MULTISAMPLE);

    // set color and depth for the glClear call
    glClearColor (0.25f, 0.25f, 0.25f, 1.0f);
    glClearDepth (1.0f);
}

Simulation::~Simulation (void)
{
}

void Simulation::Resize (const unsigned int &_width, const unsigned int &_height)
{
    // update the stored framebuffer dimensions
    width = _width;
    height = _height;

    // update the stored projection matrix and pass it to the render program
    glm::mat4 projmat = glm::perspective (45.0f, float (width) / float (height), 0.1f, 1000.0f);
    glProgramUniformMatrix4fv (renderprogram.get (), projmat_location, 1, GL_FALSE, glm::value_ptr (projmat));

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
        glProgramUniformMatrix4fv (renderprogram.get (), viewmat_location, 1, GL_FALSE,
                glm::value_ptr (camera.GetViewMatrix ()));
    }
}

void Simulation::Frame (void)
{
    // specify the viewport size
    glViewport (0, 0, width, height);

    // clear the color and depth buffer
    glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // render the framing
    renderprogram.Use ();
    framing.Render ();

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
