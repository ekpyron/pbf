Position Based Fluids
=====================
Dependencies
------------
 - GLFW (http://www.glfw.org/)
 - glm (http://glm.g-truc.net/)
 - libpng (http://libpng.org/pub/png/libpng.html)
 - cmake (http://www.cmake.org/)
 - _optional_: doxygen (http://doxygen.org/)

Compiling
---------
Create a build directory and use the following commands to compile the project:

	cmake [path to source directory]
	make

This will create the executable `src/pbf`. Note that it has to be executed with the
subdirectories _shaders_ and _textures_ in its working directory. This documentation
can be generated with `make doc`.
