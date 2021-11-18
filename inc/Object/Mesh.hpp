#ifndef _MESH_HPP_
# define _MESH_HPP_

#include "mft/mft.hpp"
#include <vector>

struct Vertex
{
	mft::vec3	pos;
	mft::vec3	norm;
	mft::vec2	uv;
};

struct Texture
{
	unsigned int	id;
	std::string		type;
};

class Mesh
{
	public:
		Mesh();
		Mesh(Mesh const & mesh);
		// Most important constructor, is going to be used
		// to initiate meshes from assimp
		Mesh(std::vector<Vertex> vertices,
			std::vector<unsigned int> indices,
			std::vector<Texture> textures);
		~Mesh();
		Mesh & operator=(Mesh const& mesh);

		void	draw() const;

		// Accessors

		unsigned int const	getVAO();
		unsigned int const	getVBO();
		unsigned int const	getEBO();

		unsigned int const	getPolygonMode();

	private:
		std::vector<Vertex>			vertices;
		std::vector<unsigned int>	indices;
		std::vector<Texture>		textures;

		std::string	name;

		unsigned int	VAO, VBO, EBO;

		unsigned int	polygon_mode;

		//		Initalize buffers and vertex array for the mesh
		//		Should not be called outside of the class scope
		void	setup();
};

	

#endif