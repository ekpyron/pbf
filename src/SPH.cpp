/*
 * Copyright (c) 2013-2014 Daniel Kirchner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE ANDNONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "SPH.h"

SPH::SPH(GLuint _numparticles, const glm::ivec3 &gridsize)
        : numparticles(_numparticles), vorticityconfinement(false), radixsort(512, _numparticles >> 9, gridsize),
          neighbourcellfinder(_numparticles, gridsize), num_solveriterations(5),_grid_size(gridsize) {
    // shader definitions
    std::stringstream stream;
    stream << "const vec3 GRID_SIZE = vec3 (" << gridsize.x << ", " << gridsize.y << ", " << gridsize.z << ");"
           << std::endl
           << "const ivec3 GRID_HASHWEIGHTS = ivec3 (1, " << gridsize.x * gridsize.z << ", " << gridsize.x << ");"
           << std::endl
           << std::endl
           #ifdef SPH_CONSTANT_PARAMETERS
           << "const float one_over_rho_0 = 1.0;" << std::endl
		   << "const float epsilon = 5.0;" << std::endl
		   << "const float gravity = 10;" << std::endl
		   << "const float timestep = 0.016;" << std::endl
		   << std::endl
		   << "const float tensile_instability_k = 0.1;" << std::endl
		   << "const float tensile_instability_scale = " << 1.0f / Wpoly6 (0.2f) << ";" << std::endl
		   << std::endl
		   << "const float xsph_viscosity_c = 0.01;" << std::endl
		   << "const float vorticity_epsilon = 5;" << std::endl
           #else
           << "layout (binding = 2, std140) uniform SPHParameters" << std::endl
           << "{" << std::endl
           << "  float one_over_rho_0;" << std::endl
           << "  float epsilon;" << std::endl
           << "  float gravity;" << std::endl
           << "  float timestep;" << std::endl
           << "  float tensile_instability_k;" << std::endl
           << "  float tensile_instability_scale;" << std::endl
           << "  float xsph_viscosity_c;" << std::endl
           << "  float vorticity_epsilon;" << std::endl
           << "};" << std::endl
           #endif
           << "const float h = 2.0;" << std::endl
           << std::endl
           << "#define BLOCKSIZE 256" << std::endl;

    // prepare shader programs
    predictpos.CompileShader(GL_COMPUTE_SHADER, {"shaders/sph/foreachneighbour.glsl", "shaders/sph/predictpos.glsl"},
                             stream.str());
    predictpos.Link();

    calclambdaprog.CompileShader(GL_COMPUTE_SHADER, {"shaders/sph/foreachneighbour.glsl", "shaders/sph/calclambda.glsl"},
                                 stream.str());
    calclambdaprog.Link();

    updateposprog.CompileShader(GL_COMPUTE_SHADER, {"shaders/sph/foreachneighbour.glsl", "shaders/sph/updatepos.glsl"},
                                stream.str());
    updateposprog.Link();

    vorticityprog.CompileShader(GL_COMPUTE_SHADER, {"shaders/sph/foreachneighbour.glsl", "shaders/sph/vorticity.glsl"},
                                stream.str());
    vorticityprog.Link();

    updateprog.CompileShader(GL_COMPUTE_SHADER, {"shaders/sph/foreachneighbour.glsl", "shaders/sph/update.glsl"},
                             stream.str());
    updateprog.Link();

    highlightprog.CompileShader(GL_COMPUTE_SHADER, {"shaders/sph/foreachneighbour.glsl", "shaders/sph/highlight.glsl"},
                                stream.str());
    highlightprog.Link();

    clearhighlightprog.CompileShader(GL_COMPUTE_SHADER, {"shaders/sph/foreachneighbour.glsl",
                                                         "shaders/sph/clearhighlight.glsl"},
                                     stream.str());
    clearhighlightprog.Link();

    // create query objects
    glGenQueries(queries.size(), queries.data());

    // create buffer objects
    glCreateBuffers(buffers.size(), buffers.data());

    // allocate lambda buffer
    glNamedBufferStorage(lambdabuffer, sizeof(float) * numparticles, nullptr, 0);

    // create lambda texture
    lambdatexture.Bind(GL_TEXTURE_BUFFER);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, lambdabuffer);

    // allocate highlight buffer
    glNamedBufferStorage(_highlight_buffer, sizeof(GLuint) * numparticles, nullptr, 0);
    glClearNamedBufferData(_highlight_buffer, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

    // create highlight buffer
    highlighttexture.Bind(GL_TEXTURE_BUFFER);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, _highlight_buffer);

    // allocate vorticity buffer
    glNamedBufferStorage(vorticitybuffer, sizeof(float) * numparticles, nullptr, 0);

    // allocate species buffer
    glNamedBufferStorage(_species_buffer, sizeof(uint8_t) * numparticles, nullptr, 0);
    _species_texture.Bind(GL_TEXTURE_BUFFER);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, _species_buffer);

    // allocate position buffer
    glNamedBufferStorage(_position_buffer, 4 * sizeof(float) * numparticles, nullptr, 0);

    // create position texture
    positiontexture.Bind(GL_TEXTURE_BUFFER);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _position_buffer);

    // allocate velocity buffer
    glNamedBufferStorage(_velocity_buffer, 4 * sizeof(float) * numparticles, nullptr, 0);

    // create velocity texture
    velocitytexture.Bind(GL_TEXTURE_BUFFER);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _velocity_buffer);

    // create sph parameter buffer
#ifndef SPH_CONSTANT_PARAMETERS
    sphparams.one_over_rho_0 = 1.0f;
    sphparams.epsilon = 5.0f;
    sphparams.gravity = 10.0f;
    sphparams.timestep = 0.016f;
    sphparams.tensile_instability_k = 0.1f;
    sphparams.tensile_instability_scale = 1.0f / Wpoly6(0.2f, 2.0f);
    sphparams.xsph_viscosity_c = 0.01f;
    sphparams.vorticity_epsilon = 5;

    glBindBuffer(GL_UNIFORM_BUFFER, sphparambuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(sphparams_t), &sphparams, GL_STATIC_DRAW);

    glBindBufferBase(GL_UNIFORM_BUFFER, 2, sphparambuffer);
#endif
}

SPH::~SPH(void) {
    // cleanup
    glDeleteBuffers(buffers.size(), buffers.data());
    glDeleteQueries(queries.size(), queries.data());
}

float SPH::Wpoly6(const float &r, const float &h) {
    if (r > h)
        return 0;
    float tmp = h * h - r * r;
    return 1.56668147106f * tmp * tmp * tmp / (h * h * h * h * h * h * h * h * h);
}

void SPH::SetRestDensity(const float &rho) {
    sphparams.one_over_rho_0 = 1.0f / rho;
    UploadSPHParams();
}

void SPH::SetCFMEpsilon(const float &epsilon) {
    sphparams.epsilon = epsilon;
    UploadSPHParams();
}

void SPH::SetGravity(const float &gravity) {
    sphparams.gravity = gravity;
    UploadSPHParams();
}

void SPH::SetTimestep(const float &timestep) {
    sphparams.timestep = timestep;
    UploadSPHParams();
}

void SPH::SetTensileInstabilityK(const float &k) {
    sphparams.tensile_instability_k = k;
    UploadSPHParams();
}

void SPH::SetTensileInstabilityScale(const float &v) {
    sphparams.tensile_instability_scale = v;
    UploadSPHParams();
}

void SPH::SetXSPHViscosity(const float &v) {
    sphparams.xsph_viscosity_c = v;
    UploadSPHParams();
}

void SPH::SetVorticityEpsilon(const float &epsilon) {
    sphparams.vorticity_epsilon = epsilon;
    UploadSPHParams();
}

void SPH::UploadSPHParams(void) {
#ifndef SPH_CONSTANT_PARAMETERS
    GLuint tmpbuffer;
    glGenBuffers(1, &tmpbuffer);
    glBindBuffer(GL_COPY_READ_BUFFER, tmpbuffer);
    glBufferData(GL_COPY_READ_BUFFER, sizeof(sphparams_t), &sphparams, GL_STREAM_COPY);
    glBindBuffer(GL_COPY_WRITE_BUFFER, sphparambuffer);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(sphparams_t));
    glDeleteBuffers(1, &tmpbuffer);
#endif
}

void SPH::OutputTiming(void) {
    GLint64 v;
    if (glIsQuery(predictposquery)) {
        glGetQueryObjecti64v(predictposquery, GL_QUERY_RESULT, &v);
        std::cout << "Position prediction: " << double(v) / 1000000.0 << " ms" << std::endl;
    }
    if (glIsQuery(sortquery)) {
        glGetQueryObjecti64v(sortquery, GL_QUERY_RESULT, &v);
        std::cout << "Sorting: " << double(v) / 1000000.0 << " ms" << std::endl;
    }
    if (glIsQuery(neighbourcellquery)) {
        glGetQueryObjecti64v(neighbourcellquery, GL_QUERY_RESULT, &v);
        std::cout << "Neighbour cell search: " << double(v) / 1000000.0 << " ms" << std::endl;
    }
    if (glIsQuery(solverquery)) {
        glGetQueryObjecti64v(solverquery, GL_QUERY_RESULT, &v);
        std::cout << "Solver: " << double(v) / 1000000.0 << " ms" << std::endl;
    }
    if (glIsQuery(vorticityquery)) {
        glGetQueryObjecti64v(vorticityquery, GL_QUERY_RESULT, &v);
        std::cout << "Vorticity confinement: " << double(v) / 1000000.0 << " ms" << std::endl;
    }
}

void SPH::SetExternalForce(bool state) {
    glProgramUniform1i(predictpos.get(), predictpos.GetUniformLocation("extforce"), state ? 1 : 0);
}

void SPH::Run(void) {
    glBeginQuery(GL_TIME_ELAPSED, predictposquery);
    {
        // predict positions
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, radixsort.GetBuffer());

        positiontexture.Bind(GL_TEXTURE_BUFFER);
        glActiveTexture(GL_TEXTURE1);
        velocitytexture.Bind(GL_TEXTURE_BUFFER);
        glActiveTexture(GL_TEXTURE0);

        predictpos.Use();
        glDispatchCompute(numparticles >> 8, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    glEndQuery(GL_TIME_ELAPSED);

    glBeginQuery(GL_TIME_ELAPSED, sortquery);
    {
        // sort particles
        radixsort.Run();
    }
    glEndQuery(GL_TIME_ELAPSED);

    {
        GLuint tmpbuffer;
        glCreateBuffers(1, &tmpbuffer);
        glNamedBufferStorage(tmpbuffer, sizeof(float) * 4 * numparticles, nullptr, GL_MAP_READ_BIT);

        glCopyNamedBufferSubData(radixsort.GetBuffer(), tmpbuffer, 0, 0, sizeof(float) * 4 * numparticles);

        const glm::vec4 *data = reinterpret_cast<const glm::vec4*>(glMapNamedBuffer(tmpbuffer, GL_READ_ONLY));

        for (auto i = 0U; i < numparticles - 1; i++) {

            auto data1 = glm::clamp(glm::ivec3(data[i]), glm::ivec3(0,0,0), _grid_size);
            auto data2 = glm::clamp(glm::ivec3(data[i + 1]), glm::ivec3(0,0,0), _grid_size);
            auto key1 = glm::dot(glm::vec3(data1), glm::vec3(grid_hashweights()));
            auto key2 = glm::dot(glm::vec3(data2), glm::vec3(grid_hashweights()));

            if (key2 < key1) {
                spdlog::get("console")->critical("Bäh {}: {} < {}", i, key2, key1);
                spdlog::get("console")->critical("  ({},{},{}) ; ({},{},{})", data[i].x, data[i].y, data[i].z, data[i + 1].x, data[i + 1].y, data[i + 1].z);
            }
        }

        glUnmapNamedBuffer(tmpbuffer);

        glDeleteBuffers(1, &tmpbuffer);
    }

    glBeginQuery(GL_TIME_ELAPSED, neighbourcellquery);
    {
        // find neighbour cells
        neighbourcellfinder.FindNeighbourCells(radixsort.GetBuffer());
    }
    glEndQuery(GL_TIME_ELAPSED);

    glBeginQuery(GL_TIME_ELAPSED, solverquery);
    {
        // set buffer bindings
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, radixsort.GetBuffer());

        glActiveTexture(GL_TEXTURE2);
        neighbourcellfinder.GetResult().Bind(GL_TEXTURE_BUFFER);
        glActiveTexture(GL_TEXTURE3);
        lambdatexture.Bind(GL_TEXTURE_BUFFER);
        glActiveTexture(GL_TEXTURE0);

        // particle highlighting
        glBindImageTexture(0, highlighttexture.get(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
        // clear previously highlighted neighbours
        clearhighlightprog.Use();
        glDispatchCompute(numparticles >> 8, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        // highlight current neighbours
        highlightprog.Use();
        glDispatchCompute(numparticles >> 8, 1, 1);

        glBindImageTexture(0, lambdatexture.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);


        // solver iteration

        for (int iteration = 0; iteration < num_solveriterations; iteration++) {
            calclambdaprog.Use();
            glDispatchCompute(numparticles >> 8, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT
                            | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            updateposprog.Use();
            glDispatchCompute(numparticles >> 8, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }
    glEndQuery(GL_TIME_ELAPSED);

    glBeginQuery(GL_TIME_ELAPSED, vorticityquery);
    {
        // update positions and velocities
        updateprog.Use();
        glBindImageTexture(0, positiontexture.get(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(1, velocitytexture.get(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glDispatchCompute(numparticles >> 8, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT
                        | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        if (vorticityconfinement) {
            // calculate vorticity
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vorticitybuffer);
            vorticityprog.Use();
            glDispatchCompute(numparticles >> 8, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }
    glEndQuery(GL_TIME_ELAPSED);
}
