#include "Blur.h"

Blur::Blur (void)
{
	program.CompileShader (GL_FRAGMENT_SHADER, "shaders/blur/fragment.glsl");
	program.CompileShader (GL_VERTEX_SHADER, "shaders/blur/vertex.glsl");
	program.Link ();

	offsetscaleloc = program.GetUniformLocation ("offsetscale");
}

Blur::~Blur (void)
{
}

/** Compute blur weights.
 * Computes gaussian blur weights using binomial coefficients.
 * \param n n
 * \param k k
 */
float ComputeWeight (unsigned long n, unsigned long k)
{
    long double tmp;
    /* scale down by the sum of all coefficients except the
     * two smallest */
    tmp = 1.0 / (powl (2, n) - 2 * (1 + n));
    /* multiply by the binomial coefficient */
    for (int i = 1; i <= k; i++)
    {
        tmp *= (n - k + i);
        tmp /= i;
    }
    return tmp;
}

void Blur::ComputeWeights (const GLuint &target, float sigma)
{
    GLuint size;

    size = sigma * 6;
    size &= ~3;
    size++;
    std::vector<float> data;

    std::vector<float> weights_data;

    /* computes binomial coefficients */
    for (int i = 0; i < (size+1)/2; i++)
    {
        weights_data.push_back
             (ComputeWeight (size + 3, ((size - 1) / 2) - i + 2));
    }

    /* push first weight and offset */
    data.push_back (weights_data[0]);
    data.push_back (0);

    /* compute weights and offsets for linear sampling */
    for (int i = 1; i <= weights_data.size() >> 1; i++)
    {
        float weight;
        /* compute and push combined weight */
        weight = weights_data[i * 2 - 1] +
             weights_data[i * 2];
        data.push_back (weight);
        /* compute and push combined offset */
        data.push_back (((i * 2 - 1) /* discrete offset */
                * weights_data[i * 2 - 1] /* discrete weight */
                + i * 2 /* discrete offset */
                * weights_data[i * 2]) /* discrete weight */
                / weight); /* scale */
    }

    glBufferData (target, sizeof (float) * data.size (), data.data (), GL_STATIC_DRAW);
}

void Blur::Apply (const glm::vec2 &offsetscale, const GLuint &weights)
{
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, weights);
    program.Use ();
    glProgramUniform2f (program.get (), offsetscaleloc, offsetscale.x, offsetscale.y);
    FullscreenQuad::Get ().Render ();
}
