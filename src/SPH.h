#ifndef SPH_H
#define SPH_H

#include "common.h"
#include "ShaderProgram.h"
#include "NeighbourCellFinder.h"
#include "RadixSort.h"

/** SPH class.
 * This class is responsible for the SPH simulation.
 */
class SPH
{
public:
	/** Constructor.
	 * \param numparticles number of particles in the simulation
	 * \param gridsize size of the particle grid
	 */
	SPH (const GLuint &numparticles, const glm::ivec3 &gridsize = glm::ivec3 (128, 64, 128));
	/** Destructor.
	 */
	~SPH (void);

	/** Get position buffer.
	 * Returns a buffer object containing the particle positions.
	 * \returns the position buffer
	 */
	GLuint GetPositionBuffer (void) const {
		return positionbuffer;
	}

	/** Get rest density.
	 * Returns the current rest density.
	 * \returns the rest density
	 */
	float GetRestDensity (void) const {
		return 1.0f / sphparams.one_over_rho_0;
	}

	/** Set rest density.
	 * Specifies the rest density.
	 * \param rho the new rest density
	 */
	void SetRestDensity (const float &rho);

	/** Get CFM epsilon.
	 * Returns the constraint force mixing epsilon.
	 * \returns the CFM epsilon
	 */
	const float &GetCFMEpsilon (void) const {
		return sphparams.epsilon;
	}

	/** Specify CFM epsilon.
	 * Specifies the constraint force mixing epsilon.
	 * \param epsilon the CFM epsilon
	 */
	void SetCFMEpsilon (const float &epsilon);

	/** Get gravity.
	 * Returns the gravity strength.
	 * \returns the gravity strength
	 */
	const float &GetGravity (void) const {
		return sphparams.gravity;
	}

	/** Set gravity.
	 * Specifies the gravity strength.
	 * \param gravity the gravity strength
	 */
	void SetGravity (const float &gravity);

	/** Get time step.
	 * Returns the simulation time step.
	 * \returns the simulation time step.
	 */
	const float &GetTimestep (void) const {
		return sphparams.timestep;
	}

	/** Set time step.
	 * Specifies the simulation time step.
	 * \param timestep the simulation time step.
	 */
	void SetTimestep (const float &timestep);

	/** Poly6 kernel.
	 * Evaluate the Poly6 kernel for a given radius.
	 * Used to specify the tensile instability scale factor
	 * in terms of the smoothing kernel.
	 * \param r radius
	 * \param h smoothing width
	 * \returns smoothing weight
	 */
	static float Wpoly6 (const float &r, const float &h);

	/** Get tensile instability K.
	 * Returns tensile instability K.
	 * \returns tensile instability K.
	 */
	const float &GetTensileInstabilityK (void) const {
		return sphparams.tensile_instability_k;
	}

	/** Set tensile instability K.
	 * \param k tensile instability K.
	 */
	void SetTensileInstabilityK (const float &k);

	/** Get tensile instability scale.
	 * Returns tensile instability scale.
	 * \returns tensile instability scale.
	 */
	const float &GetTensileInstabilityScale (void) const {
		return sphparams.tensile_instability_scale;
	}

	/** Set tensile instability scale.
	 * \param v tensile instability scale.
	 */
	void SetTensileInstabilityScale (const float &v);
	/** Get velocity buffer.
	 * Returns a buffer object containing the particle velocities.
	 * \returns the velocity buffer
	 */
	GLuint GetVelocityBuffer (void) const {
		return velocitybuffer;
	}

	/** Get highlight buffer.
	 * Returns a buffer object containing the particle highlighting information.
	 * \returns the highlight buffer
	 */
	GLuint GetHighlightBuffer (void) const {
		return highlightbuffer;
	}

	/** Check vorticity confinement.
	 * Checks the state of the vorticity confinement.
	 * \returns True, if vorticity confinement is enabled, false, if not.
	 */
	const bool &IsVorticityConfinementEnabled (void) const {
		return vorticityconfinement;
	}
	/** Enable/disable vorticity confinement.
	 * Specifies whether to use vorticity confinement or not.
	 * \param flag Flag indicating whether to use vorticity confinement.
	 */
	void SetVorticityConfinementEnabled (const bool &flag) {
		vorticityconfinement = flag;
	}

	/** Activate/deactivate an external force.
	 * Activates or deactivates an external force in negative z direction
	 * that is applied to all particles with a z-coordinate larger than
	 * half the grid size.
	 * \param state true to enable the external force, false to disable it
	 */
	void SetExternalForce (bool state);

	/** Run simulation.
	 * Runs the SPH simulation.
	 */
	void Run (void);

	/** Output timing information.
	 * Outputs timing information about the simulation steps to the
	 * standard output.
	 */
	void OutputTiming (void);
private:
	/** Data type for SPH uniform parameters.
	 * This structure represents the memory layout of the uniform buffer
	 * object in which the SPH parameters are stored.
	 */
	typedef struct sphparams {
		float one_over_rho_0;
		float epsilon;
		float gravity;
		float timestep;
		float tensile_instability_k;
		float tensile_instability_scale;
		float xsph_viscosity_c;
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

    /** Highlight texture
     * Texture used to access the highlight buffer
     */
    Texture highlighttexture;

    union {
        struct {
            /** Position buffer.
             * Buffer in which the position of each particle is stored.
             */
            GLuint positionbuffer;

            /** Velocity buffer.
             * Buffer in which the velocity of each particle is stored.
             */
            GLuint velocitybuffer;

            /** Highlight buffer.
             * Buffer in which particle highlighting information is stored
             */
            GLuint highlightbuffer;

            /** Lambda buffer.
             * Buffer in which the specific scalar values are stored during the simulation steps.
             */
            GLuint lambdabuffer;

            /** Vorticity buffer.
             * Buffer in which the vorticity of each particle is stored.
             */
            GLuint vorticitybuffer;

            /** SPH parameter buffer.
             * Uniform buffer in which the SPH parameters are stored.
             */
            GLuint sphparambuffer;
        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        GLuint buffers[6];
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
        GLuint queries[5];
    };


    /** Number of particles.
     * Stores the number of particles in the simulation.
     */
    const GLuint numparticles;
};

#endif /* SPH_H */
