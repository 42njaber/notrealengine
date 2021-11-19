#ifndef _MESH_CLASS_H_
# define _MESH_CLASS_H_

#include "mft/mft.hpp"
#include <vector>

namespace notrealengine
{
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
		std::string		path;
	};

	class Mesh
	{

	public:

		std::string	name;

		Mesh();
		Mesh(Mesh const& mesh);
		// Most important constructor, is going to be used
		// to initiate meshes from assimp
		Mesh(std::vector<Vertex> vertices,
			std::vector<unsigned int> indices,
			std::vector<Texture> textures);
		~Mesh();
		Mesh& operator=(Mesh const& mesh);

		void	draw() const;

		// Accessors

		std::vector<Vertex>			getVertices() const;
		std::vector<unsigned int>	getIndices() const;
		std::vector<Texture>		getTextures() const;

		unsigned int const	getVAO() const;
		unsigned int const	getVBO() const;
		unsigned int const	getEBO() const;

		unsigned int const	getPolygonMode() const;


	private:
		std::vector<Vertex>			vertices;
		std::vector<unsigned int>	indices;
		std::vector<Texture>		textures;

		unsigned int	VAO, VBO, EBO;

		unsigned int	polygon_mode;

		//		Initalize buffers and vertex array for the mesh
		//		Should not be called outside of the class scope
		void	setup();
	};

	std::ostream& operator<<(std::ostream& o, Mesh const& mesh);
}

	

#endif