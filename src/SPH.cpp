#include "SPH.h"

SPH::SPH (const GLuint &_numparticles)
	: numparticles (_numparticles), vorticityconfinement (false), radixsort (_numparticles >> 9),
	  neighbourcellfinder (_numparticles)
{
	// prepare shader programs
    predictpos.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/predictpos.glsl",
    		"shaders/simulation/include.glsl");
    predictpos.Link ();

    calclambdaprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/calclambda.glsl",
    		"shaders/simulation/include.glsl");
    calclambdaprog.Link ();

    updateposprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/updatepos.glsl",
    		"shaders/simulation/include.glsl");
    updateposprog.Link ();

    vorticityprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/vorticity.glsl",
    		"shaders/simulation/include.glsl");
    vorticityprog.Link ();

    updateprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/update.glsl",
    		"shaders/simulation/include.glsl");
    updateprog.Link ();

    dummyprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/dummy.glsl",
    		"shaders/simulation/include.glsl");
    dummyprog.Link ();

    // create query objects
    glGenQueries (6, queries);

	// create buffer objects
	glGenBuffers (3, buffers);

    // allocate lambda buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, lambdabuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * numparticles, NULL, GL_DYNAMIC_DRAW);

    // allocate vorticity buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, vorticitybuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * numparticles, NULL, GL_DYNAMIC_DRAW);

    // allocate particle buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, particlebuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (particleinfo_t) * numparticles, NULL, GL_DYNAMIC_DRAW);

}

SPH::~SPH (void)
{
	// cleanup
	glDeleteBuffers (3, buffers);
	glDeleteQueries (6, queries);
}

void SPH::OutputTiming (void)
{
	GLint64 v;
	if (glIsQuery (preparationquery))
	{
		glGetQueryObjecti64v (preparationquery, GL_QUERY_RESULT, &v);
		std::cout << "Preparation: " << double (v) / 1000000.0 << " ms" << std::endl;
	}
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
	glBeginQuery (GL_TIME_ELAPSED, preparationquery);
	{
		// clear lambda buffer
		glBindBuffer (GL_SHADER_STORAGE_BUFFER, lambdabuffer);
		glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, NULL);
		glMemoryBarrier (GL_BUFFER_UPDATE_BARRIER_BIT);
	}
	glEndQuery (GL_TIME_ELAPSED);

    glBeginQuery (GL_TIME_ELAPSED, predictposquery);
    {
    	// predict positions
    	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, radixsort.GetBuffer ());
    	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, particlebuffer);
    	predictpos.Use ();
    	glDispatchCompute (numparticles >> 8, 1, 1);
    	glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
    }
    glEndQuery (GL_TIME_ELAPSED);

    glBeginQuery (GL_TIME_ELAPSED, sortquery);
    {
    	// sort particles
    	radixsort.Run (20);
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
        // use neighbour cell texture as input
    	neighbourcellfinder.GetResult ().Bind (GL_TEXTURE_BUFFER);

        // set buffer bindings
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, radixsort.GetBuffer ());
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, particlebuffer);
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, lambdabuffer);

    	// solver iteration
        dummyprog.Use ();
        glDispatchCompute (numparticles >> 8, 1, 1);

    	for (int iteration = 0; iteration < 5; iteration++)
    	{
    		calclambdaprog.Use ();
    		glDispatchCompute (numparticles >> 8, 1, 1);
    		glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
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
		glDispatchCompute (numparticles >> 8, 1, 1);
		glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
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
