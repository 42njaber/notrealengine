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

	GLMesh::GLMesh(GLMesh && GLMesh) :
		name(std::move(GLMesh.name)),
		textures(std::move(GLMesh.textures)),
		VAO(std::exchange(GLMesh.VAO, 0)),
		VBO(std::exchange(GLMesh.VBO, 0)),
		EBO(std::exchange(GLMesh.EBO, 0)),
		nbIndices(std::exchange(GLMesh.nbIndices, 0)),
		polygonMode(std::exchange(GLMesh.polygonMode, 0))
	{

	}

	GLMesh::~GLMesh()
	{
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
	}

	GLMesh::GLMesh(MeshData const & data, std::vector<std::shared_ptr<Texture>> textures)
		: name(""),
		textures(textures),
		VAO(0), VBO(0), EBO(0),
		polygonMode(GL_FILL),
		nbIndices(0)
	{
		setup(data);
	}

	GLMesh& GLMesh::operator=(GLMesh && GLMesh)
	{
		this->name = std::move(GLMesh.name);

		this->textures = std::move(GLMesh.textures);

		this->VBO = std::exchange(GLMesh.VBO, 0);
		this->VAO = std::exchange(GLMesh.VAO, 0);
		this->EBO = std::exchange(GLMesh.EBO, 0);

		this->polygonMode = std::exchange(GLMesh.polygonMode, 0);

		this->nbIndices = std::exchange(GLMesh.nbIndices, 0);
		return *this;
	}

	//	Accessors

	std::vector<std::shared_ptr<Texture>> const &	GLMesh::getTextures() const
	{
		return textures;
	}

	unsigned int const &	GLMesh::getVAO() const
	{
		return VAO;
	}

	unsigned int const& GLMesh::getVBO() const
	{
		return VBO;
	}

	unsigned int const& GLMesh::getEBO() const
	{
		return EBO;
	}

	unsigned int const& GLMesh::getPolygonMode() const
	{
		return polygonMode;
	}

	std::string const& GLMesh::getName() const
	{
		return name;
	}

	//	Setters

	void	GLMesh::setName(std::string name)
	{
		this->name = name;
	}

	//	Texture utility

	void	GLMesh::addTexture(std::shared_ptr < Texture >& text)
	{
		textures.push_back(std::move(text));
	}

	//	Main functions

	void	GLMesh::draw(unsigned int shader, mft::mat4 transform, mft::mat4 normalMatrix) const
	{
		unsigned int	diffuse = 0;
		unsigned int	specular = 0;
		//GLCallThrow(glUseProgram, shader);
		GLCallThrow(glUniformMatrix4fv, GLCallThrow(glGetUniformLocation, shader, "model"), 1, GL_TRUE, static_cast<float*>(transform));
		GLCallThrow(glUniformMatrix4fv, GLCallThrow(glGetUniformLocation, shader, "normalMatrix"), 1, GL_TRUE, static_cast<float*>(normalMatrix));
		for (size_t i = 0; i < textures.size(); i++)
		{
			GLCallThrow(glActiveTexture, GL_TEXTURE0 + (unsigned int)i);

			std::string	nb;
			std::string	name = textures[i]->getType();
			if (name == "material.diffuse")
				nb = std::to_string(diffuse);
			else if (name == "material.specular")
				nb = std::to_string(specular);
			GLCallThrow(glBindTexture, GL_TEXTURE_2D, textures[i]->getGLId());
			GLCallThrow(glUniform1i, GLCallThrow(glGetUniformLocation, shader, name.c_str()), i);
		}
		GLCallThrow(glActiveTexture, GL_TEXTURE0);
		GLCallThrow(glBindVertexArray, VAO);
		GLCallThrow(glPolygonMode, GL_FRONT_AND_BACK, polygonMode);
		GLCallThrow(glDrawElements, GL_TRIANGLES, nbIndices, GL_UNSIGNED_INT, 0);
		GLCallThrow(glBindVertexArray, 0);
	}

	//		Initalize buffers and vertex array for the GLMesh
	void	GLMesh::setup(MeshData const& data)
	{
		GLCallThrow(glGenVertexArrays, 1, &VAO);
		GLCallThrow(glGenBuffers, 1, &VBO);
		GLCallThrow(glGenBuffers, 1, &EBO);

		GLCallThrow(glBindVertexArray, VAO);

		GLCallThrow(glBindBuffer, GL_ARRAY_BUFFER, VBO);
		GLCallThrow(glBufferData, GL_ARRAY_BUFFER,
			data.getVertices().size() * sizeof(Vertex),
			&data.getVertices()[0], GL_STATIC_DRAW);

		GLCallThrow(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, EBO);
		GLCallThrow(glBufferData, GL_ELEMENT_ARRAY_BUFFER,
			data.getIndices().size() * sizeof(unsigned int),
			data.getIndices().data(), GL_STATIC_DRAW);

		nbIndices = data.getIndices().size();

		//	Pos
		GLCallThrow(glVertexAttribPointer,
			0, 3, GL_FLOAT, GL_FALSE,
			sizeof(Vertex), (void*)0);
		GLCallThrow(glEnableVertexAttribArray, 0);

		//	Norm
		GLCallThrow(glVertexAttribPointer,
			1, 3, GL_FLOAT, GL_FALSE,
			sizeof(Vertex), (void*)offsetof(Vertex, norm));
		GLCallThrow(glEnableVertexAttribArray, 1);

		//	UV
		GLCallThrow(glVertexAttribPointer,
			2, 2, GL_FLOAT, GL_FALSE,
			sizeof(Vertex), (void*)offsetof(Vertex, uv));
		GLCallThrow(glEnableVertexAttribArray, 2);

		//	Bone ID
		GLCallThrow(glVertexAttribIPointer,
			3, MAX_BONE_INFLUENCE, GL_INT,
			sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));
		GLCallThrow(glEnableVertexAttribArray, 3);

		//	Bone weight
		GLCallThrow(glVertexAttribPointer,
			4, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE,
			sizeof(Vertex), (void*)offsetof(Vertex, weights));
		GLCallThrow(glEnableVertexAttribArray, 4);

		GLCallThrow(glBindVertexArray, 0);
	}

	std::ostream& operator<<(std::ostream& o, GLMesh const& GLMesh)
	{
		return o;
	}
}
