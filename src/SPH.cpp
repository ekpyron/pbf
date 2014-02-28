#include "SPH.h"

SPH::SPH (const GLuint &_numparticles, const glm::ivec3 &gridsize)
	: numparticles (_numparticles), vorticityconfinement (false), radixsort (512, _numparticles >> 9, gridsize),
	  neighbourcellfinder (_numparticles, gridsize)
{
	// shader definitions
	std::stringstream stream;
	stream << "#version 430 core" << std::endl
		   << "const vec3 GRID_SIZE = vec3 (" << gridsize.x << ", " << gridsize.y << ", " << gridsize.z << ");" << std::endl
		   << "const ivec3 GRID_HASHWEIGHTS = ivec3 (1, " << gridsize.x * gridsize.z <<  ", " << gridsize.z << ");" << std::endl
		   << std::endl
		   << "const float rho_0 = 1.0;" << std::endl
		   << "const float h = 2.0;" << std::endl
		   << "const float epsilon = 5.0;" << std::endl
		   << "const float gravity = 10;" << std::endl
		   << "const float timestep = 0.016;" << std::endl
		   << std::endl
		   << "const float tensile_instability_k = 0.1;" << std::endl
		   << "const float tensile_instability_h = 0.2;" << std::endl
		   << std::endl
		   << "const float xsph_viscosity_c = 0.01;" << std::endl
		   << "const float vorticity_epsilon = 5;" << std::endl
		   << std::endl
		   << "#define BLOCKSIZE 256" << std::endl;

	// prepare shader programs
    predictpos.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/predictpos.glsl", stream.str ());
    predictpos.Link ();

    calclambdaprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/calclambda.glsl", stream.str ());
    calclambdaprog.Link ();

    updateposprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/updatepos.glsl", stream.str ());
    updateposprog.Link ();

    vorticityprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/vorticity.glsl", stream.str ());
    vorticityprog.Link ();

    updateprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/update.glsl", stream.str ());
    updateprog.Link ();

    highlightprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/highlight.glsl", stream.str ());
    highlightprog.Link ();

    clearhighlightprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/clearhighlight.glsl", stream.str ());
    clearhighlightprog.Link ();

    // create query objects
    glGenQueries (5, queries);

	// create buffer objects
	glGenBuffers (5, buffers);

    // allocate lambda buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, lambdabuffer);
   	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * numparticles, NULL, GL_DYNAMIC_COPY);

    // create lambda texture
    lambdatexture.Bind (GL_TEXTURE_BUFFER);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_R32F, lambdabuffer);

    // allocate highlight buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, highlightbuffer);
   	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLuint) * numparticles, NULL, GL_DYNAMIC_COPY);
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, NULL);

    // create highlight buffer
    highlighttexture.Bind (GL_TEXTURE_BUFFER);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_R32UI, highlightbuffer);

    // allocate vorticity buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, vorticitybuffer);
   	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * numparticles, NULL, GL_DYNAMIC_COPY);

    // allocate position buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, positionbuffer);
   	glBufferData (GL_SHADER_STORAGE_BUFFER, 4 * sizeof (float) * numparticles, NULL, GL_DYNAMIC_COPY);

    // create position texture
    positiontexture.Bind (GL_TEXTURE_BUFFER);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_RGBA32F, positionbuffer);

    // allocate velocity buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, velocitybuffer);
   	glBufferData (GL_SHADER_STORAGE_BUFFER, 4 * sizeof (float) * numparticles, NULL, GL_DYNAMIC_COPY);

    // create velocity texture
    velocitytexture.Bind (GL_TEXTURE_BUFFER);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_RGBA32F, velocitybuffer);
}

SPH::~SPH (void)
{
	// cleanup
	glDeleteBuffers (5, buffers);
	glDeleteQueries (5, queries);
}

void SPH::OutputTiming (void)
{
	GLint64 v;
	if (glIsQuery (predictposquery))
	{
		glGetQueryObjecti64v (predictposquery, GL_QUERY_RESULT, &v);
		std::cout << "Position prediction: " << double (v) / 1000000.0 << " ms" << std::endl;
	}
	if (glIsQuery (sortquery))
	{
		glGetQueryObjecti64v (sortquery, GL_QUERY_RESULT, &v);
		std::cout << "Sorting: " << double (v) / 1000000.0 << " ms" << std::endl;
	}
	if (glIsQuery (neighbourcellquery))
	{
		glGetQueryObjecti64v (neighbourcellquery, GL_QUERY_RESULT, &v);
		std::cout << "Neighbour cell search: " << double (v) / 1000000.0 << " ms" << std::endl;
	}
	if (glIsQuery (solverquery))
	{
		glGetQueryObjecti64v (solverquery, GL_QUERY_RESULT, &v);
		std::cout << "Solver: " << double (v) / 1000000.0 << " ms" << std::endl;
	}
	if (glIsQuery (vorticityquery))
	{
		glGetQueryObjecti64v (vorticityquery, GL_QUERY_RESULT, &v);
		std::cout << "Vorticity confinement: " << double (v) / 1000000.0 << " ms" << std::endl;
	}
}

void SPH::SetExternalForce (bool state)
{
	glProgramUniform1i (predictpos.get (), predictpos.GetUniformLocation ("extforce"), state ? 1 : 0);
}

void SPH::Run (void)
{
    glBeginQuery (GL_TIME_ELAPSED, predictposquery);
    {
    	// predict positions
    	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, radixsort.GetBuffer ());

    	positiontexture.Bind (GL_TEXTURE_BUFFER);
    	glActiveTexture (GL_TEXTURE1);
    	velocitytexture.Bind (GL_TEXTURE_BUFFER);
    	glActiveTexture (GL_TEXTURE0);

    	predictpos.Use ();
    	glDispatchCompute (numparticles >> 8, 1, 1);
    	glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
    }
    glEndQuery (GL_TIME_ELAPSED);

    glBeginQuery (GL_TIME_ELAPSED, sortquery);
    {
    	// sort particles
    	radixsort.Run ();
    }
    glEndQuery (GL_TIME_ELAPSED);

    glBeginQuery (GL_TIME_ELAPSED, neighbourcellquery);
    {
    	// find neighbour cells
    	neighbourcellfinder.FindNeighbourCells (radixsort.GetBuffer ());
    }
    glEndQuery (GL_TIME_ELAPSED);

    glBeginQuery (GL_TIME_ELAPSED, solverquery);
    {
        // set buffer bindings
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, radixsort.GetBuffer ());

        glActiveTexture (GL_TEXTURE2);
    	neighbourcellfinder.GetResult ().Bind (GL_TEXTURE_BUFFER);
        glActiveTexture (GL_TEXTURE3);
        lambdatexture.Bind (GL_TEXTURE_BUFFER);
        glActiveTexture (GL_TEXTURE0);

        // particle highlighting
        glBindImageTexture (0, highlighttexture.get (), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
        // clear previously highlighted neighbours
        clearhighlightprog.Use ();
        glDispatchCompute (numparticles >> 8, 1, 1);
        glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        // highlight current neighbours
        highlightprog.Use ();
        glDispatchCompute (numparticles >> 8, 1, 1);

        glBindImageTexture (0, lambdatexture.get (), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);


    	// solver iteration

    	for (int iteration = 0; iteration < 5; iteration++)
    	{
    		calclambdaprog.Use ();
    		glDispatchCompute (numparticles >> 8, 1, 1);
    		glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT|GL_TEXTURE_FETCH_BARRIER_BIT
    				|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        	updateposprog.Use ();
    		glDispatchCompute (numparticles >> 8, 1, 1);
    		glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
    	}
    }
    glEndQuery (GL_TIME_ELAPSED);

    glBeginQuery (GL_TIME_ELAPSED, vorticityquery);
    {
		// update positions and velocities
		updateprog.Use ();
		glBindImageTexture (0, positiontexture.get (), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture (1, velocitytexture.get (), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glDispatchCompute (numparticles >> 8, 1, 1);
		glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT|GL_TEXTURE_FETCH_BARRIER_BIT
				|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    	if (vorticityconfinement)
    	{
    		// calculate vorticity
    		glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, vorticitybuffer);
    		vorticityprog.Use ();
    		glDispatchCompute (numparticles >> 8, 1, 1);
    		glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
    	}
    }
    glEndQuery (GL_TIME_ELAPSED);
}
