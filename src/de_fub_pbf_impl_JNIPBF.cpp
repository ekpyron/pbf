#include "de_fub_pbf_impl_JNIPBF.h"
#include "Simulation.h"
#include <iostream>
#include <exception>

Simulation *simulation = NULL;

JNIEXPORT jboolean JNICALL Java_de_fub_pbf_impl_JNIPBF__1resized
  (JNIEnv *, jobject, jint w, jint h)
  {
	if (simulation)
	{
		try {
			simulation->Resize (w, h);
			return true;
		} catch (std::exception &e) {
			std::cerr << "Exception: " << e.what () << std::endl;
			return false;
		}
	}
	return false;
  }
  
PFNGLMEMORYBARRIERPROC _glMemoryBarrier_BROKEN_ATIHACK = 0;
  
void _glMemoryBarrier_ATIHACK (GLbitfield bitfield)
{
	GLsync syncobj = glFenceSync (GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glWaitSync (syncobj, 0, GL_TIMEOUT_IGNORED);
	glDeleteSync (syncobj);
	_glMemoryBarrier_BROKEN_ATIHACK (bitfield);
}

JNIEXPORT jboolean JNICALL Java_de_fub_pbf_impl_JNIPBF__1init
  (JNIEnv *, jobject)
  {
  	if (glewInit () != GLEW_OK)
	{
		std::cerr << "glewInit () failed." << std::endl;
		return false;
	}
	
	std::string vendor (reinterpret_cast<const char*> (glGetString (GL_VENDOR)));
    if (!vendor.compare ("ATI Technologies Inc."))
    {
    	std::cout << "Enable ATI workarounds." << std::endl;
    	_glMemoryBarrier_BROKEN_ATIHACK = glMemoryBarrier;
    	glMemoryBarrier = _glMemoryBarrier_ATIHACK;
    }
	
	if (simulation) delete simulation;
	try {
		simulation = new Simulation (NULL);
		return true;
	} catch (std::exception &e) {
		std::cerr << "Exception: " << e.what () << std::endl;
		return false;
	}
  }

JNIEXPORT jboolean JNICALL Java_de_fub_pbf_impl_JNIPBF__1display
  (JNIEnv *, jobject)
  {
  	if (simulation)
	{
		try {
			simulation->Frame ();
			return true;
		} catch (std::exception &e) {
			std::cerr << "Exception: " << e.what () << std::endl;
			return false;
		}
	}
	else return false;

  }

JNIEXPORT jboolean JNICALL Java_de_fub_pbf_impl_JNIPBF__1start
  (JNIEnv *, jobject)
  {
	if (simulation)
	{
		try {
			simulation->Start ();
			return true;
		} catch (std::exception &e) {
			std::cerr << "Exception: " << e.what () << std::endl;
			return false;
		}
	}
	else return false;
  }

JNIEXPORT jboolean JNICALL Java_de_fub_pbf_impl_JNIPBF__1stop
  (JNIEnv *, jobject)
  {
	if (simulation)
	{
		try {
			simulation->Stop ();
			return true;
		} catch (std::exception &e) {
			std::cerr << "Exception: " << e.what () << std::endl;
			return false;
		}
	}
	else return false;
  }

JNIEXPORT jboolean JNICALL Java_de_fub_pbf_impl_JNIPBF__1zoom
  (JNIEnv *, jobject, jfloat amount)
  {
  	if (simulation)
	{
		try {
			simulation->Zoom (amount);
			return true;
		} catch (std::exception &e) {
			std::cerr << "Exception: " << e.what () << std::endl;
			return false;
		}
	}
	else return false;

  }
  
JNIEXPORT jboolean JNICALL Java_de_fub_pbf_impl_JNIPBF__1rotate
  (JNIEnv *, jobject, jfloat x, jfloat y)
{
  	if (simulation)
	{
		try {
			simulation->Rotate (x, y);
			return true;
		} catch (std::exception &e) {
			std::cerr << "Exception: " << e.what () << std::endl;
			return false;
		}
	}
	else return false;
  }
   
JNIEXPORT jboolean JNICALL Java_de_fub_pbf_impl_JNIPBF__1move
  (JNIEnv *, jobject, jfloat x, jfloat y)
  {
  	if (simulation)
	{
		try {
			simulation->Move (x, y);
			return true;
		} catch (std::exception &e) {
			std::cerr << "Exception: " << e.what () << std::endl;
			return false;
		}
	}
	else return false;
  }
 
JNIEXPORT jboolean JNICALL Java_de_fub_pbf_impl_JNIPBF__1dispose
  (JNIEnv *, jobject)
  {
	if (simulation)
	{
		delete simulation;
		simulation = NULL;
	}
	return true;
  }
