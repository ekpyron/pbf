#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <glcorew.h>

int main (int argc, char *argv[])
{
    try {
    glfwInit ();
    // specify parameters for the opengl context
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    // open a window and an OpenGL context
    GLFWwindow *window = glfwCreateWindow (1280, 720, "TEST", NULL, NULL);
    if (window == NULL)
        throw std::runtime_error ("Cannot open window.");
    glfwMakeContextCurrent (window);

    // get OpenGL entry points
    glcorewInit ((glcorewGetProcAddressCallback) glfwGetProcAddress);

    GLint program = glCreateProgram ();
    GLint shader = glCreateShader (GL_COMPUTE_SHADER);
    const GLchar *src = "#version 430 core\n"
            "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
            "layout (std430, binding = 0) buffer Buffer { float data[]; };\n"
            "void main (void)\n"
            "{\n"
            " data[0] = 42;\n"
            "}\n";
    glShaderSource (shader, 1, &src, NULL);
    glCompileShader (shader);
    glAttachShader (program, shader);
    glLinkProgram (program);
    glUseProgram (program);

    GLuint buffer;
    glGenBuffers (1, &buffer);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, buffer);
    float data = 0;

    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float), &data, GL_DYNAMIC_DRAW);

    glGetBufferSubData (GL_SHADER_STORAGE_BUFFER, 0, sizeof (float), &data);

    std::cout << "Before: " << data << std::endl;

    glDispatchCompute (1, 1, 1);

    glFinish ();
    glMemoryBarrier (GL_ALL_BARRIER_BITS);

    glGetBufferSubData (GL_SHADER_STORAGE_BUFFER, 0, sizeof (float), &data);

    std::cout << "After: " << data << std::endl;

    glfwDestroyWindow (window);
    glfwTerminate ();

    return 0;

    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what () << std::endl;
        return -1;
    }
}
