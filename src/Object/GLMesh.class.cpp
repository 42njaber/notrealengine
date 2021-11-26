#include "Object/GLMesh.class.hpp"

//	OpenGL includes
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	#include <windows.h>
	#include <glad/glad.h>
#else
	# define GL_GLEXT_PROTOTYPES
	#include <GL/gl.h>
#endif
#include <string>

namespace notrealengine
{
	//	Constructors

	GLMesh::GLMesh() : VAO(0), VBO(0), EBO(0), polygon_mode(GL_FILL)
	{
	}

	GLMesh::GLMesh(GLMesh const & GLMesh) : data(GLMesh.data), VAO(0), VBO(0), EBO(0),
		polygon_mode(GL_FILL)
	{
	}

	GLMesh::~GLMesh()
	{
	}

	GLMesh::GLMesh(MeshData const & data)
		: data(data),
		VAO(0), VBO(0), EBO(0), polygon_mode(GL_FILL)
	{
		setup();
	}

	GLMesh& GLMesh::operator=(GLMesh const& GLMesh)
	{
		this->data = GLMesh.data;

		this->name = GLMesh.name;

		this->VBO = GLMesh.VBO;
		this->VAO = GLMesh.VAO;
		this->EBO = GLMesh.EBO;

		this->polygon_mode = GLMesh.polygon_mode;
		return *this;
	}

	//	Accessors

	MeshData const		GLMesh::getData() const
	{
		return data;
	}

	unsigned int const	GLMesh::getVAO() const
	{
		return VAO;
	}

	unsigned int const	GLMesh::getVBO() const
	{
		return VBO;
	}

	unsigned int const	GLMesh::getEBO() const
	{
		return EBO;
	}

	unsigned int const	GLMesh::getPolygonMode() const
	{
		return polygon_mode;
	}

	//	Main functions

	void	GLMesh::draw(GLShaderProgram *shader) const
	{
		unsigned int	diffuse = 0;
		unsigned int	specular = 0;
		for (size_t i = 0; i < data.getTextures().size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + (unsigned int)i);

			std::string	nb;
			std::string	name = data.getTextures()[i].type;
			if (name == "texture_diffuse")
				nb = std::to_string(diffuse);
			else if (name == "texture_specular")
				nb = std::to_string(specular);
			glBindTexture(GL_TEXTURE_2D, data.getTextures()[i].id);
			glUniform1f(glGetUniformLocation(shader->programID, ("material." + name + nb).c_str()), i);
		}
		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(VAO);
		glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
		glDrawElements(GL_TRIANGLES, (int)data.getIndices().size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	//		Initalize buffers and vertex array for the GLMesh
	void	GLMesh::setup()
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glBufferData(GL_ARRAY_BUFFER,
			(GLsizeiptr)(data.getVertices().size() * sizeof(Vertex)),
			&data.getVertices()[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			(GLsizeiptr)(data.getIndices().size() * sizeof(unsigned int)),
			&data.getIndices()[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));

		glBindVertexArray(0);
	}

	std::ostream& operator<<(std::ostream& o, GLMesh const& GLMesh)
	{
		std::vector<Vertex>			vertices = GLMesh.getData().getVertices();
		std::vector<unsigned int>	indices = GLMesh.getData().getIndices();
		std::vector<Texture>		textures = GLMesh.getData().getTextures();
		std::cout << "\t--Vertices--" << std::endl;
		for (size_t i = 0; i < vertices.size(); i++)
		{
			std::cout << "\t" << vertices[i].pos << vertices[i].norm << vertices[i].uv << std::endl;
		}
		std::cout << "\t--Indices--" << std::endl;
		for (size_t i = 0; i < indices.size(); i++)
		{
			std::cout << "\t" << indices[i] << std::endl;
		}
		std::cout << "\t--Textures--" << std::endl;
		for (size_t i = 0; i < textures.size(); i++)
		{
			std::cout << "\t" << textures[i].path << std::endl;
		}
		return o;
	}
}