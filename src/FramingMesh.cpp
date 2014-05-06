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
#include "FramingMesh.h"

/**
 *  A wrapper class for the Open Asset Importer LogStream that just prints to cerr.
 */
class LogStream : public Assimp::LogStream
{
public:
	 virtual void write (const char *msg) {
		 std::cerr << msg;
	 }
};

FramingMesh::FramingMesh (const char* input)
{
	// load shader
    program.CompileShader (GL_VERTEX_SHADER, "shaders/framing/vertex.glsl");
    program.CompileShader (GL_FRAGMENT_SHADER, "shaders/framing/fragment.glsl");
    program.Link ();

	// load mesh
	const aiScene* mesh = FramingMesh::ImportMesh(input);

	if(mesh) {
	    // write data into buffer objects
	} else {
		std::cerr << "Failed at loading mesh" << std::endl;
	}
}

FramingMesh::~FramingMesh (void)
{
    // clean up
    glDeleteBuffers (4, buffers);
    glDeleteVertexArrays (1, &vertexarray);
}

void FramingMesh::Render (void)
{
	// activate shader program
	program.Use ();
    // bind texture, vertex array and index buffer
    texture.Bind (GL_TEXTURE_2D);
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    // render the framing
    glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

const aiScene* FramingMesh::ImportMesh(const char* fileName) {
	std::cout << "Attempting to import mesh " << fileName << std::endl;
	LogStream logstream;
	Assimp::DefaultLogger::create("ASSIMP_LOG", Assimp::Logger::VERBOSE);
	Assimp::DefaultLogger::get()->attachStream (&logstream,
			Assimp::Logger::Debugging	|
			Assimp::Logger::Info		|
			Assimp::Logger::Err			|
			Assimp::Logger::Warn);
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName, aiProcess_GenNormals | aiProcess_FixInfacingNormals | aiProcess_FindInvalidData);
	if(!scene) {
		std::cerr << "Cannot load the model " << fileName << std::endl;
	} else {
		std::cout << "Importing mesh succeeded" << std::endl;
	}
	return scene;
}
