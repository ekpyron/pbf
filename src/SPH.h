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
#ifndef SPH_H
#define SPH_H

#include "common.h"
#include "ShaderProgram.h"
#include "NeighbourCellFinder.h"
#include "RadixSort.h"

/** SPH class.
 * This class is responsible for the SPH simulation.
 */
class SPH {
public:
    /** Constructor.
     * \param numparticles number of particles in the simulation
     * \param gridsize size of the particle grid
     */
    SPH(GLuint numparticles, const glm::ivec3 &gridsize = glm::ivec3(128, 64, 128));

    /** Destructor.
     */
    ~SPH();

    /** Get position buffer.
     * Returns a buffer object containing the particle positions.
     * \returns the position buffer
     */
    GLuint position_buffer() const {
        return _position_buffer;
    }

    /** Get rest density.
     * Returns the current rest density.
     * \returns the rest density
     */
    float GetRestDensity() const {
        return 1.0f / sphparams.one_over_rho_0;
    }

    /** Set rest density.
     * Specifies the rest density.
     * \param rho the new rest density
     */
    void SetRestDensity(const float &rho);

    /** Get CFM epsilon.
     * Returns the constraint force mixing epsilon.
     * \returns the CFM epsilon
     */
    const float &GetCFMEpsilon() const {
        return sphparams.epsilon;
    }

    /** Specify CFM epsilon.
     * Specifies the constraint force mixing epsilon.
     * \param epsilon the CFM epsilon
     */
    void SetCFMEpsilon(const float &epsilon);

    /** Get gravity.
     * Returns the gravity strength.
     * \returns the gravity strength
     */
    const float &GetGravity() const {
        return sphparams.gravity;
    }

    /** Set gravity.
     * Specifies the gravity strength.
     * \param gravity the gravity strength
     */
    void SetGravity(const float &gravity);

    /** Get time step.
     * Returns the simulation time step.
     * \returns the simulation time step.
     */
    const float &GetTimestep() const {
        return sphparams.timestep;
    }

    /** Set time step.
     * Specifies the simulation time step.
     * \param timestep the simulation time step.
     */
    void SetTimestep(const float &timestep);

    /** Poly6 kernel.
     * Evaluate the Poly6 kernel for a given radius.
     * Used to specify the tensile instability scale factor
     * in terms of the smoothing kernel.
     * \param r radius
     * \param h smoothing width
     * \returns smoothing weight
     */
    static float Wpoly6(const float &r, const float &h);

    /** Get tensile instability K.
     * Returns tensile instability K.
     * \returns tensile instability K.
     */
    const float &GetTensileInstabilityK() const {
        return sphparams.tensile_instability_k;
    }

    /** Set tensile instability K
     * Specifies the tensile instability K.
     * \param k tensile instability K.
     */
    void SetTensileInstabilityK(const float &k);

    /** Get tensile instability scale.
     * Returns tensile instability scale.
     * \returns tensile instability scale.
     */
    const float &GetTensileInstabilityScale() const {
        return sphparams.tensile_instability_scale;
    }

    /** Set tensile instability scale.
     * \param v tensile instability scale.
     */
    void SetTensileInstabilityScale(const float &v);

    /** Get XSPH viscosity.
     * Returns the XSPH viscosity constant.
     * \returns the XSPH viscosity constant.
     *
     */
    const float &GetXSPHViscosity() const {
        return sphparams.xsph_viscosity_c;
    }

    /** Set XSPH viscosity.
     * Specifies the XSPH viscosity.
     * \param v XSPH viscosity
     */
    void SetXSPHViscosity(const float &v);

    /** Get vorticity epsilon.
     * Returns the vorticity confinement epsilon.
     * \returns the vorticity confinement epsilon.
     *
     */
    const float &GetVorticityEpsilon() const {
        return sphparams.vorticity_epsilon;
    }

    /** Set Vorticity epsilon.
     * Specifies the vorticity confinement epsilon.
     * \param epsilon the vorticity confinement epsilon.
     *
     */
    void SetVorticityEpsilon(const float &epsilon);

    /** Get velocity buffer.
     * Returns a buffer object containing the particle velocities.
     * \returns the velocity buffer
     */
    GLuint velocity_buffer() const {
        return _velocity_buffer;
    }

    /** Get number of solver iterations.
     * Returns the number of solver iterations currently used.
     * \returns the number of solver iterations.
     */
    const GLuint &GetNumSolverIterations() const {
        return num_solveriterations;
    }

    /** Set number of solver iterations.
     * Specifies the number of solver iterations.
     * \param iter the number of solver iterations.
     */
    void SetNumSolverIterations(const GLuint &iter) {
        num_solveriterations = iter;
    }

    /** Get highlight buffer.
     * Returns a buffer object containing the particle highlighting information.
     * \returns the highlight buffer
     */
    GLuint highlight_buffer() const {
        return _highlight_buffer;
    }

    GLuint species_buffer() const {
        return _species_buffer;
    }

    /** Check vorticity confinement.
     * Checks the state of the vorticity confinement.
     * \returns True, if vorticity confinement is enabled, false, if not.
     */
    const bool &IsVorticityConfinementEnabled() const {
        return vorticityconfinement;
    }

    /** Enable/disable vorticity confinement.
     * Specifies whether to use vorticity confinement or not.
     * \param flag Flag indicating whether to use vorticity confinement.
     */
    void SetVorticityConfinementEnabled(const bool &flag) {
        vorticityconfinement = flag;
    }

    /** Activate/deactivate an external force.
     * Activates or deactivates an external force in negative z direction
     * that is applied to all particles with a z-coordinate larger than
     * half the grid size.
     * \param state true to enable the external force, false to disable it
     */
    void SetExternalForce(bool state);

    /** Run simulation.
     * Runs the SPH simulation.
     */
    void Run();

    /** Output timing information.
     * Outputs timing information about the simulation steps to the
     * standard output.
     */
    void OutputTiming();

private:
    /** Upload SPH parameters.
     * Uploads the SPH parameter buffer to the contents of the sphparams
     * structure to the GPU.
     */
    void UploadSPHParams();

    /** Data type for SPH uniform parameters.
     * This structure represents the memory layout of the uniform buffer
     * object in which the SPH parameters are stored.
     */
    typedef struct sphparams {
        /** One over rest density.
         * One over the rest density.
         */
        float one_over_rho_0;
        /** CFM epsilon.
         * Constraint force mixing epsilon.
         */
        float epsilon;
        /** Gravity.
         * Magnitude of the gravity.
         */
        float gravity;
        /** Timestep.
         * Simulation timestep.
         */
        float timestep;
        /** Tensile instability K.
         * A parameter for the tensile instability calculations.
         */
        float tensile_instability_k;
        /** Tensile instability scale.
         * A parameter for the tensile instability calculations.
         */
        float tensile_instability_scale;
        /** XSPH viscosity factor.
         * A parameter for the XSPH viscosity calculations.
         */
        float xsph_viscosity_c;
        /** Vorticity confinement epsilon.
         * A parameter for the vorticity confinement.
         */
        float vorticity_epsilon;
    } sphparams_t;

    /** SPH uniform parameters.
     * This structure stores a copy of the contents of the uniform buffer
     * in which the SPH parameters are stored.
     */
    sphparams_t sphparams;

    /** Simulation step shader program.
     * Shader program for the simulation step that predicts the new position
     * of all particles.
     */
    ShaderProgram predictpos;

    /** Simulation step shader program.
     * Shader program for the simulation step that calculates lambda_i for each particle.
     */
    ShaderProgram calclambdaprog;

    /** Simulation step shader program.
     * Shader program for the simulation step that updates the position of each particle
     * according to the calculated lambdas.
     */
    ShaderProgram updateposprog;

    /** Vorticity program.
     * Shader program for calculating particle vorticity.
     */
    ShaderProgram vorticityprog;

    /** Update program.
     * Shader program for updating particle information.
     * This is done by the vorticity program, if vorticity confinement is enabled.
     */
    ShaderProgram updateprog;

    /** Highlight program.
     * Shader program for highlighing neighbours of highlighted particles.
     */
    ShaderProgram highlightprog;

    /** Clear highlight program.
     * Shader program for clearing the neighbour highlights.
     */
    ShaderProgram clearhighlightprog;


    /** Neighbour Cell finder.
     * Takes care of finding neighbour cells for the particles.
     */
    NeighbourCellFinder neighbourcellfinder;

    /** Vorticity confinement flag.
     * flag indicating whether vorticity confinement should be used.
     */
    bool vorticityconfinement;

    /** Radix sort.
     * Takes care of sorting the particle list.
     * The contained buffer object is used as particle buffer.
     */
    RadixSort radixsort;

    /** Lambda texture.
     * Texture used to access the lambda buffer.
     */
    Texture lambdatexture;

    /** Position texture.
     * Texture used to access the position buffer.
     */
    Texture positiontexture;

    /** Velocity texture.
     * Texture used to access the velocity buffer.
     */
    Texture velocitytexture;

    /** Species texture.
     * Texture used to access the species buffer.
     */
    Texture _species_texture;

    /** Highlight texture
     * Texture used to access the highlight buffer
     */
    Texture highlighttexture;

    /** Number of solver iterations.
     * Number of solver iterations used for the constraint solver.
     */
    GLuint num_solveriterations;

    union {
        struct {
            /** Position buffer.
             * Buffer in which the position of each particle is stored.
             */
            GLuint _position_buffer;

            /** Velocity buffer.
             * Buffer in which the velocity of each particle is stored.
             */
            GLuint _velocity_buffer;

            /** Highlight buffer.
             * Buffer in which particle highlighting information is stored
             */
            GLuint _highlight_buffer;

            /** Lambda buffer.
             * Buffer in which the specific scalar values are stored during the simulation steps.
             */
            GLuint lambdabuffer;

            /** Vorticity buffer.
             * Buffer in which the vorticity of each particle is stored.
             */
            GLuint vorticitybuffer;

            /** Species buffer.
             * Buffer in which the species of each particle is stored.
             */
            GLuint _species_buffer;

            /** SPH parameter buffer.
             * Uniform buffer in which the SPH parameters are stored.
             */
            GLuint sphparambuffer;
        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        std::array<GLuint, 7> buffers;
    };

    union {
        struct {
            /** Predict position query object.
             * Query object to record the time spent in the predict position phase.
             */
            GLuint predictposquery;
            /** Sorting query object.
             * Query object to record the time spent in the sorting phase.
             */
            GLuint sortquery;
            /** Neighbour cell query object.
             * Query object to record the time spent in the neighbour cell phase.
             */
            GLuint neighbourcellquery;
            /** Solver query object.
             * Query object to record the time spent in the solver phase.
             */
            GLuint solverquery;
            /** Vorticity query object.
             * Query object to record the time spent in the vorticity confinement phase.
             */
            GLuint vorticityquery;
        };
        /** Query objects.
         * The query objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        std::array<GLuint, 5> queries;
    };

    glm::ivec3 _grid_size;
    glm::ivec3 grid_hashweights() const{
        return glm::ivec3(1, _grid_size.x * _grid_size.z, _grid_size.x);
    };

    /** Number of particles.
     * Stores the number of particles in the simulation.
     */
    const GLuint numparticles;
};

#endif /* SPH_H */
