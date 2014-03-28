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
