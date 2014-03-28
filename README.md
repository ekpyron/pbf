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

References
----------
This position based fluids implementation is based on the following scientific paper:

http://www.matthiasmueller.info/publications/pbf_sig_preprint.pdf


Furthermore techniques from the following papers have been used:

http://www.sci.utah.edu/~csilva/papers/cgf.pdf

http://beowulf.lcs.mit.edu/18.337-2008/lectslides/scan.pdf

http://developer.download.nvidia.com/presentations/2010/gdc/Direct3D_Effects.pdf

Documentation
-------------

More extensive documentation than the current doxygen generated reference will be
available shortly.

Demonstration videos of a former version of the implementation can currently be
found at (they were recorded using a NVIDIA GeForce GTX 560 Ti):
http://mokaga.userpage.fu-berlin.de/pbfvids/
