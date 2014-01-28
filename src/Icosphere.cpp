#include "Icosphere.h"

/** Get edge midpoint.
 * Obtains the index of the midpoint of two vertices
 * and maintains a cache of already calculated midpoints.
 * \param vertices list of vertices
 * \param cache midpoint cache
 * \param a start vertex
 * \param b end vertex
 * \returns index of the midpoint
 */
int getMidpoint (std::vector<glm::vec3> &vertices, std::map<std::pair<int, int>, int> &cache, int a, int b)
{
	std::pair<int, int> key;
	if (a < b) key = std::make_pair (a, b);
	else key = std::make_pair (b, a);
	std::map<std::pair<int, int>, int>::iterator it = cache.find (key);
	if (it == cache.end ())
	{
		glm::vec3 v = glm::normalize (0.5f * (vertices[a] + vertices[b]));
		int id = vertices.size ();
		vertices.push_back (v);
		cache[key] = id;
		return id;
	}
	return it->second;
}

Icosphere::Icosphere (unsigned int subdivsions)
{
    // create and bind a vertex array
    glGenVertexArrays (1, &vertexarray);
    glBindVertexArray (vertexarray);

    // generate buffer objects
    glGenBuffers (2, buffers);

    // generate icosahedron vertices
    std::vector<glm::vec3> vertices;
    float t = (1.0f + sqrtf (5.0f)) / 2.0f;
    vertices.push_back (glm::vec3 (-1, t, 0));
    vertices.push_back (glm::vec3 ( 1, t, 0));
    vertices.push_back (glm::vec3 (-1,-t, 0));
    vertices.push_back (glm::vec3 ( 1,-t, 0));
    vertices.push_back (glm::vec3 ( 0,-1, t));
    vertices.push_back (glm::vec3 ( 0, 1, t));
    vertices.push_back (glm::vec3 ( 0,-1,-t));
    vertices.push_back (glm::vec3 ( 0, 1,-t));
    vertices.push_back (glm::vec3 ( t, 0,-1));
    vertices.push_back (glm::vec3 ( t, 0, 1));
    vertices.push_back (glm::vec3 (-t, 0,-1));
    vertices.push_back (glm::vec3 (-t, 0, 1));

    // normalize vertices
    for (int i = 0; i < vertices.size (); i++)
    	vertices[i] = glm::normalize (vertices[i]);

    // generate icosahedron faces
    std::vector<glm::u16vec3> faces;
    faces.push_back (glm::u16vec3 (0, 11, 5));
    faces.push_back (glm::u16vec3 (0, 5, 1));
    faces.push_back (glm::u16vec3 (0, 1, 7));
    faces.push_back (glm::u16vec3 (0, 7, 10));
    faces.push_back (glm::u16vec3 (0, 10, 11));
    faces.push_back (glm::u16vec3 (1, 5, 9));
    faces.push_back (glm::u16vec3 (5, 11, 4));
    faces.push_back (glm::u16vec3 (11, 10, 2));
    faces.push_back (glm::u16vec3 (10, 7, 6));
    faces.push_back (glm::u16vec3 (7, 1, 8));
    faces.push_back (glm::u16vec3 (3, 9, 4));
    faces.push_back (glm::u16vec3 (3, 4, 2));
    faces.push_back (glm::u16vec3 (3, 2, 6));
    faces.push_back (glm::u16vec3 (3, 6, 8));
    faces.push_back (glm::u16vec3 (3, 8, 9));
    faces.push_back (glm::u16vec3 (4, 9, 5));
    faces.push_back (glm::u16vec3 (2, 4, 11));
    faces.push_back (glm::u16vec3 (6, 2, 10));
    faces.push_back (glm::u16vec3 (8, 6, 7));
    faces.push_back (glm::u16vec3 (9, 8, 1));

    // subdivide icosahedron
    {
    	std::map<std::pair<int, int>, int> cache;
    	for (int i = 0; i < subdivsions; i++)
    	{
    		std::vector<glm::u16vec3> newfaces;
    		newfaces.reserve (faces.size () * 4);
    		for (int i = 0; i < faces.size (); i++)
    		{
    			const glm::u16vec3 &f = faces[i];
    			int a = getMidpoint (vertices, cache, f.x, f.y);
    			int b = getMidpoint (vertices, cache, f.y, f.z);
    			int c = getMidpoint (vertices, cache, f.z, f.x);
    			newfaces.push_back (glm::u16vec3 (f.x, a, c));
    			newfaces.push_back (glm::u16vec3 (f.y, b, a));
    			newfaces.push_back (glm::u16vec3 (f.z, c, b));
    			newfaces.push_back (glm::u16vec3 (a, b, c));
    		}
    		std::swap (faces, newfaces);
    	}
    }
    numindices = 3 * faces.size ();

    // generate normalized 16-bit vertices
    std::vector<GLshort> svertices;
    for (int i = 0; i < vertices.size (); i++)
    {
    	svertices.push_back (vertices[i].x * 32767);
    	svertices.push_back (vertices[i].y * 32767);
    	svertices.push_back (vertices[i].z * 32767);
    	svertices.push_back (0);
    }

    // store vertices to a buffer object
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData (GL_ARRAY_BUFFER, sizeof (GLshort) * svertices.size (), &svertices[0], GL_STATIC_DRAW);
    // define the vertices as vertex attribute 0
    glVertexAttribPointer (0, 3, GL_SHORT, GL_TRUE, 8, 0);
    glEnableVertexAttribArray (0);

    // store indices to a buffer object
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (glm::u16vec3) * faces.size (), &faces[0], GL_STATIC_DRAW);
}

Icosphere::~Icosphere (void)
{
    // cleanup
    glDeleteBuffers (2, buffers);
    glDeleteVertexArrays (1, &vertexarray);
}

void Icosphere::SetPositionBuffer (GLuint buffer, GLsizei stride, GLintptr offset)
{
    // bind the vertex array and the position buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ARRAY_BUFFER, buffer);
    // define the per-instance vertex attribute
    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, stride, (const GLvoid*) offset);
    glEnableVertexAttribArray (1);
    glVertexAttribDivisor (1, 1);
}

void Icosphere::SetColorBuffer (GLuint buffer, GLsizei stride, GLintptr offset)
{
    // bind the vertex array and the color buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ARRAY_BUFFER, buffer);
    // define the per-instance vertex attribute
    glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, stride, (const GLvoid*) offset);
    glEnableVertexAttribArray (2);
    glVertexAttribDivisor (2, 1);
}

void Icosphere::Render (GLuint instances) const
{
    // bind vertex array and index buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    // render the framing
    glDrawElementsInstanced (GL_TRIANGLES, numindices, GL_UNSIGNED_SHORT, 0, instances);
}
