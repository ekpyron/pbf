#include "RadixSort.h"
#include <random>

RadixSort::RadixSort (GLuint _numblocks)
	: blocksize (512), numblocks (_numblocks)
{
	counting.CompileShader (GL_COMPUTE_SHADER, "shaders/radixsort/counting.glsl", "shaders/simulation/include.glsl");
	counting.Link ();
	blockscan.CompileShader (GL_COMPUTE_SHADER, "shaders/radixsort/blockscan.glsl", "shaders/simulation/include.glsl");
	blockscan.Link ();
	globalsort.CompileShader (GL_COMPUTE_SHADER, "shaders/radixsort/globalsort.glsl", "shaders/simulation/include.glsl");
	globalsort.Link ();
	addblocksum.CompileShader (GL_COMPUTE_SHADER, "shaders/radixsort/addblocksum.glsl", "shaders/simulation/include.glsl");
	addblocksum.Link ();

	glGenBuffers (3, buffers);

	glBindBuffer (GL_SHADER_STORAGE_BUFFER, buffer);
	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (particleinfo_t) * blocksize * numblocks, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, prefixsums);
	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (uint32_t) * blocksize * numblocks, NULL, GL_DYNAMIC_DRAW);

	uint32_t numblocksums = 4 * numblocks;
	{
		int n = ceil (log (((numblocksums + blocksize - 1) / blocksize) * blocksize) / log (blocksize));
		n++;
		blocksums.resize (n);
		glGenBuffers (n, &blocksums[0]);
	}
	for (GLuint &blocksum : blocksums)
	{
		glBindBuffer (GL_SHADER_STORAGE_BUFFER, blocksum);
		numblocksums = ((numblocksums + blocksize - 1) / blocksize) * blocksize;
		if (numblocksums < 1)
			numblocksums = 1;
		glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (uint32_t) * numblocksums, NULL, GL_STATIC_DRAW);
		numblocksums /= blocksize;
	}

	glBindBuffer (GL_SHADER_STORAGE_BUFFER, result);
	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (particleinfo_t) * blocksize * numblocks, NULL, GL_STATIC_DRAW);

	glm::uvec4 blocksumoffsets (0, numblocks, numblocks * 2, numblocks * 3);

	glProgramUniform4uiv (counting.get (), counting.GetUniformLocation ("blocksumoffsets"), 1,
			glm::value_ptr (blocksumoffsets));
	glProgramUniform4uiv (globalsort.get (), globalsort.GetUniformLocation ("blocksumoffsets"), 1,
			glm::value_ptr (blocksumoffsets));

	counting_bitshift = counting.GetUniformLocation ("bitshift");
	globalsort_bitshift = globalsort.GetUniformLocation ("bitshift");

	for (int blocksum : blocksums)
	{
		glBindBuffer (GL_SHADER_STORAGE_BUFFER, blocksum);
		glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32UI, GL_RGBA, GL_UNSIGNED_INT, NULL);
	}
	glMemoryBarrier (GL_BUFFER_UPDATE_BARRIER_BIT);
}

RadixSort::~RadixSort (void)
{
	glDeleteBuffers (blocksums.size (), &blocksums[0]);
	glDeleteBuffers (3, buffers);
}

GLuint RadixSort::GetBuffer (void) const
{
	return buffer;
}

void RadixSort::Run (unsigned int numbits)
{
	for (int i = 0; i < (numbits + 1) >> 1; i++)
	{
		SortBits (2 * i);
		std::swap (result, buffer);
	}
}

uint32_t intpow (uint32_t x, uint32_t y)
{
	uint32_t r = 1;
	while (y)
	{
		if (y & 1)
			r *= x;
		y >>= 1;
		x *= x;
	}
	return r;
}

void RadixSort::SortBits (int bits)
{
	glProgramUniform1i (counting.get (), counting_bitshift, bits);
	glProgramUniform1i (globalsort.get (), globalsort_bitshift, bits);

	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, buffer);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, prefixsums);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, blocksums.front ());
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, result);

	counting.Use ();
	glDispatchCompute (numblocks, 1, 1);
	glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);

	blockscan.Use ();
	uint32_t numblocksums = (4 * numblocks) / blocksize;
	for (int i = 0; i < blocksums.size () - 1; i++)
	{
		glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, blocksums[i]);
		glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, blocksums[i + 1]);
		glDispatchCompute (numblocksums > 0 ? numblocksums : 1, 1, 1);
		numblocksums /= blocksize;
		glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
	}

	addblocksum.Use ();
	for (int i = blocksums.size () - 3; i >= 0; i--)
	{
		uint32_t numblocksums = (4 * numblocks) / intpow (blocksize, i + 1);
		glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, blocksums[i]);
		glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, blocksums[i + 1]);
		glDispatchCompute (numblocksums > 0 ? numblocksums : 1, 1, 1);
		glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
	}

	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, buffer);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, prefixsums);

	globalsort.Use ();
	glDispatchCompute (numblocks, 1, 1);
	glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
}
