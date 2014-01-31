#include "SPH.h"

SPH::SPH (const GLuint &_numparticles)
	: numparticles (_numparticles), vorticityconfinement (false), radixsort (_numparticles >> 9),
	  neighbourcellfinder (_numparticles)
{
	// prepare shader programs
    predictpos.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/predictpos.glsl",
    		"shaders/simulation/include.glsl");
    predictpos.Link ();

    solver.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/solver.glsl",
    		"shaders/simulation/include.glsl");
    solver.Link ();

    vorticityprog.CompileShader (GL_COMPUTE_SHADER, "shaders/sph/vorticity.glsl",
    		"shaders/simulation/include.glsl");
    vorticityprog.Link ();

    // create query objects
    glGenQueries (6, queries);

	// create buffer objects
	glGenBuffers (2, buffers);

    // allocate lambda buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, lambdabuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * numparticles, NULL, GL_DYNAMIC_DRAW);

    // allocate auxillary buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, auxbuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * 4 * numparticles, NULL, GL_DYNAMIC_DRAW);
    // clear auxiliary buffer
    const float auxdata[] = { 0.25, 0, 1, 1 };
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &auxdata[0]);

}

SPH::~SPH (void)
{
	// cleanup
	glDeleteBuffers (2, buffers);
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

void SPH::Run (void)
{
	glBeginQuery (GL_TIME_ELAPSED, preparationquery);
	{
		// clear lambda buffer
		glBindBuffer (GL_SHADER_STORAGE_BUFFER, lambdabuffer);
		glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, NULL);

		// clear auxiliary buffer
		glBindBuffer (GL_SHADER_STORAGE_BUFFER, auxbuffer);
		const float auxdata[] = { 0.25, 0, 1, 1 };
		glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &auxdata[0]);
		glMemoryBarrier (GL_BUFFER_UPDATE_BARRIER_BIT);
	}
	glEndQuery (GL_TIME_ELAPSED);

    glBeginQuery (GL_TIME_ELAPSED, predictposquery);
    {
    	// predict positions
    	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, radixsort.GetBuffer ());
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
        glBindTexture (GL_TEXTURE_BUFFER, neighbourcellfinder.GetResult ());

        // set buffer bindings
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, radixsort.GetBuffer ());
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, lambdabuffer);
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, auxbuffer);

    	// solver iteration
    	solver.Use ();
    	for (auto i = 0; i < 5; i++)
    	{
    		glDispatchCompute (numparticles >> 8, 1, 1);
    		glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
    	}
    }
    glEndQuery (GL_TIME_ELAPSED);

    glBeginQuery (GL_TIME_ELAPSED, vorticityquery);
    {
    	// calculate vorticity
    	if (vorticityconfinement)
    	{
    		vorticityprog.Use ();
    		glDispatchCompute (numparticles >> 8, 1, 1);
    	}
    }
    glEndQuery (GL_TIME_ELAPSED);
}
