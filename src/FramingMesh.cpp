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

FramingMesh::FramingMesh (const char* input) : num_indices (0)
{
	// load shader
    program.CompileShader (GL_VERTEX_SHADER, "shaders/framingmesh/vertex.glsl");
    program.CompileShader (GL_FRAGMENT_SHADER, "shaders/framingmesh/fragment.glsl");
    program.Link ();

    // create and bind a vertex array
    glGenVertexArrays (1, &vertexarray);

    // generate buffer objects
    glGenBuffers (3, buffers);

	// load mesh
    LoadMesh (input);
}

FramingMesh::~FramingMesh (void)
{
    // clean up
    glDeleteBuffers (3, buffers);
    glDeleteVertexArrays (1, &vertexarray);
}

void FramingMesh::LoadMesh (const std::string &fileName)
{
	// bind vertex array
    glBindVertexArray (vertexarray);

	Assimp::Importer importer;
	aiMesh *mesh;
    {
    	std::cout << "Attempting to import mesh " << fileName << std::endl;
    	LogStream logstream;
    	Assimp::DefaultLogger::create("ASSIMP_LOG", Assimp::Logger::VERBOSE);
    	Assimp::DefaultLogger::get()->attachStream (&logstream,
    			Assimp::Logger::Debugging	|
    			Assimp::Logger::Info		|
    			Assimp::Logger::Err			|
    			Assimp::Logger::Warn);
    	const aiScene* scene = importer.ReadFile(fileName, aiProcess_GenSmoothNormals | aiProcess_FixInfacingNormals | aiProcess_FindInvalidData | aiProcess_Triangulate);
    	if(!scene)
    		throw std::runtime_error (std::string ("Cannot load the model ") + fileName);
    	if (!scene->mNumMeshes)
    		throw std::runtime_error ("No mesh found in the specified file.");
    	if (scene->mNumMeshes > 1)
    		std::cerr << "Warning: more than one mesh found in the specified file. Using the first mesh." << std::endl;
    	mesh = scene->mMeshes[0];
    	std::cout << "Importing mesh succeeded" << std::endl;
    }

    std::vector<glm::vec3> vertices;
    {
    	// compute axis aligned bounding box
    	glm::vec3 min = glm::make_vec3 (&mesh->mVertices[0].x);
    	glm::vec3 max = min;
    	for (int i = 1; i < mesh->mNumVertices; i++)
    	{
    		glm::vec3 cur = glm::make_vec3 (&mesh->mVertices[i].x);
    		for (int j = 0; j < 3; j++)
    		{
    			if (cur[j] < min[j])
    				min[j] = cur[j];
    			if (cur[j] > max[j])
    				max[j] = cur[j];
    		}
    	}

    	// determine scale and translation
    	float dist = glm::distance (min, max);
    	if (dist == 0)
    		throw std::runtime_error ("Mesh has no spatial extent.");
    	float scale = 64.0f / dist;
    	glm::vec3 translate = -0.5f * (min + max) + glm::vec3 (0, max.y, 0);

    	vertices.reserve (mesh->mNumVertices);
    	for (int i = 0; i < mesh->mNumVertices; i++)
    	{
    		glm::vec3 cur = glm::make_vec3 (&mesh->mVertices[i].x);
    		vertices.push_back (scale * (cur + translate));
    	}
    }

    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData (GL_ARRAY_BUFFER, vertices.size () * sizeof (glm::vec3), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (0);


    glBindBuffer (GL_ARRAY_BUFFER, normalbuffer);
    glBufferData (GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof (glm::vec3), mesh->mNormals, GL_STATIC_DRAW);
    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (1);

    {
    	std::vector<GLuint> indices;
    	for (int i = 0; i < mesh->mNumFaces; i++)
    	{
    		const aiFace &face = mesh->mFaces[i];
    		if (face.mNumIndices != 3)
    			throw std::runtime_error ("Mesh contains non triangular faces.");
    		indices.insert (indices.end (), &face.mIndices[0], &face.mIndices[3]);
    	}
    	glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    	glBufferData (GL_ELEMENT_ARRAY_BUFFER, indices.size () * sizeof (GLuint), &indices[0], GL_STATIC_DRAW);
    	num_indices = indices.size ();
    }
}

void FramingMesh::Render (void)
{
	// activate shader program
	program.Use ();
    // bind vertex array and index buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    // render the framing
    glDisable (GL_CULL_FACE);
    glDrawElements (GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
    glEnable (GL_CULL_FACE);
}
