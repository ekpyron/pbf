#ifndef RADIXSORT_H
#define RADIXSORT_H

#include "common.h"
#include "ShaderProgram.h"

class RadixSort
{
public:
	 RadixSort (GLuint numblocks);
	 ~RadixSort (void);
	 GLuint GetBuffer (void);
	 void Run (unsigned int numbits);
private:
	 void SortBits (int bits);
	 ShaderProgram counting;
	 ShaderProgram blockscan;
	 ShaderProgram globalsort;
	 ShaderProgram addblocksum;
	 union {
		 struct {
			 GLuint buffer;
			 GLuint prefixsums;
			 GLuint result;
		 };
		 GLuint buffers[3];
	 };
	 std::vector<GLuint> blocksums;

	 uint32_t blocksize;
	 uint32_t numblocks;
	 int counting_bitshift;
	 int globalsort_bitshift;
};

#endif /* !defined RADIXSORT_H */
