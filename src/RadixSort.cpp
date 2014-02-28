#include "RadixSort.h"

unsigned int count_sortbits (uint64_t v)
{
	unsigned int r = 1;
	while (v >>= 1)
		r++;
	return r;
}

RadixSort::RadixSort (GLuint _blocksize, GLuint _numblocks, const glm::ivec3 &gridsize)
	: blocksize (_blocksize), numblocks (_numblocks)
{
	std::stringstream stream;
	stream << "#version 430 core" << std::endl
		   << "const vec3 GRID_SIZE = vec3 (" << gridsize.x << ", " << gridsize.y << ", " << gridsize.z << ");" << std::endl
		   << "const ivec3 GRID_HASHWEIGHTS = ivec3 (1, " << gridsize.x * gridsize.z <<  ", " << gridsize.z << ");" << std::endl
		   << "#define BLOCKSIZE " << blocksize << std::endl
		   << "#define HALFBLOCKSIZE " << (blocksize / 2) << std::endl;

	if (blocksize & 1)
		throw std::logic_error ("The block size for sorting has to be even.");

	numbits = count_sortbits (uint64_t (gridsize.x) * uint64_t (gridsize.y) * uint64_t (gridsize.z) - 1);

	// load shaders
	counting.CompileShader (GL_COMPUTE_SHADER, "shaders/radixsort/counting.glsl", stream.str ());
	counting.Link ();
	blockscan.CompileShader (GL_COMPUTE_SHADER, "shaders/radixsort/blockscan.glsl", stream.str ());
	blockscan.Link ();
	globalsort.CompileShader (GL_COMPUTE_SHADER, "shaders/radixsort/globalsort.glsl", stream.str ());
	globalsort.Link ();
	addblocksum.CompileShader (GL_COMPUTE_SHADER, "shaders/radixsort/addblocksum.glsl", stream.str ());
	addblocksum.Link ();

	// create buffer objects
	glGenBuffers (3, buffers);

	// allocate input buffer
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, buffer);
	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (glm::vec4) * blocksize * numblocks, NULL, GL_DYNAMIC_COPY);

	// allocate prefix sum buffer
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, prefixsums);
	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (uint32_t) * blocksize * numblocks, NULL, GL_DYNAMIC_COPY);

	// create block sum buffers
	uint32_t numblocksums = 4 * numblocks;
	{
		int n = ceil (log (((numblocksums + blocksize - 1) / blocksize) * blocksize) / log (blocksize));
		n++;
		blocksums.resize (n);
		glGenBuffers (n, &blocksums[0]);
	}
	for (int i = 0; i < blocksums.size (); i++)
	{
		GLuint &blocksum = blocksums[i];
		// allocate a single block sum buffer
		glBindBuffer (GL_SHADER_STORAGE_BUFFER, blocksum);
		numblocksums = ((numblocksums + blocksize - 1) / blocksize) * blocksize;
		if (numblocksums < 1)
			numblocksums = 1;
		glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (uint32_t) * numblocksums, NULL, GL_DYNAMIC_COPY);
		numblocksums /= blocksize;
	}

	// allocate output buffer
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, result);
	glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (glm::vec4) * blocksize * numblocks, NULL, GL_DYNAMIC_COPY);

	// pass block sum offsets to the shader programs
	glm::uvec4 blocksumoffsets (0, numblocks, numblocks * 2, numblocks * 3);
	glProgramUniform4uiv (counting.get (), counting.GetUniformLocation ("blocksumoffsets"), 1,
			glm::value_ptr (blocksumoffsets));
	glProgramUniform4uiv (globalsort.get (), globalsort.GetUniformLocation ("blocksumoffsets"), 1,
			glm::value_ptr (blocksumoffsets));

	// query uniform locations
	counting_bitshift = counting.GetUniformLocation ("bitshift");
	globalsort_bitshift = globalsort.GetUniformLocation ("bitshift");

	// clear block sum buffers
	for (int i = 0; i < blocksums.size (); i++)
	{
		unsigned int &blocksum = blocksums[i];
		glBindBuffer (GL_SHADER_STORAGE_BUFFER, blocksum);
		glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32UI, GL_RGBA, GL_UNSIGNED_INT, NULL);
	}
	glMemoryBarrier (GL_BUFFER_UPDATE_BARRIER_BIT);
}

RadixSort::~RadixSort (void)
{
	// cleanup
	glDeleteBuffers (blocksums.size (), &blocksums[0]);
	glDeleteBuffers (3, buffers);
}

GLuint RadixSort::GetBuffer (void) const
{
	// return the current input buffer
	return buffer;
}

void RadixSort::Run (void)
{
	// sort bits from least to most significant
	for (int i = 0; i < (numbits + 1) >> 1; i++)
	{
		SortBits (2 * i);
		// swap the buffer objects
		std::swap (result, buffer);
	}
}

/** Integer power.
 * Calculates an integer power.
 * \param x base
 * \param y exponent
 * \returns x to the power of y
 */
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
	// pass current bit shift to the shader programs
	glProgramUniform1i (counting.get (), counting_bitshift, bits);
	glProgramUniform1i (globalsort.get (), globalsort_bitshift, bits);

	// set buffer bindings
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, buffer);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, prefixsums);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, blocksums.front ());
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, result);

	// counting
	counting.Use ();
	glDispatchCompute (numblocks, 1, 1);
	glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);

	// create block sums level by level
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

	// add block sums level by level (in reversed order)
	addblocksum.Use ();
	for (int i = blocksums.size () - 3; i >= 0; i--)
	{
		uint32_t numblocksums = (4 * numblocks) / intpow (blocksize, i + 1);
		glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, blocksums[i]);
		glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, blocksums[i + 1]);
		glDispatchCompute (numblocksums > 0 ? numblocksums : 1, 1, 1);
		glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
	}

	// map values to their global position in the output buffer
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, buffer);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, prefixsums);
	globalsort.Use ();
	glDispatchCompute (numblocks, 1, 1);
	glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
}
