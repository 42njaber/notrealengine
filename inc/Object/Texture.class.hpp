#ifndef _TEXTURE_CLASS_H_
# define _TEXTURE_CLASS_H_

#include <string>
#include <set>

#include "mft/mft.hpp"
#include "GLShaderProgram.class.hpp"

namespace notrealengine
{
	class Texture
	{
	public:
		Texture() = delete;
		Texture(Texture const& ref) = delete;
		Texture(Texture && ref) noexcept;
		Texture(std::string const& path, std::string const& type);
		~Texture();
		Texture& operator=(Texture const& text) = delete;
		Texture& operator=(Texture && text) noexcept;

		//	Accessors

		unsigned int const& getId() const;
		std::string const& getType() const;

		//	Setters

		void	setType(std::string const& type);

		void
			draw(GLShaderProgram* shader, mft::vec2 pos,
				mft::vec2 size, float rotation, mft::vec3 color) const;

	private:
		unsigned int	glId, VAO, VBO;
		std::string		type;

	};
}

#endif