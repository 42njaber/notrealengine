
#ifndef  __GLCONTEXT_CLASS_H__
# define __GLCONTEXT_CLASS_H__

# include "GLShaderProgram.class.hpp"

# include <string>
# include <filesystem>
# include <map>

# include "GL.hpp"
# include "Object/GLMesh.class.hpp"

namespace notrealengine
{

	/*
	 *  This class works as an interface, could be instantiated
	 *  using different libraries in the future
	 */
	class GLContext {

	public:
		virtual void
			makeCurrent( void ) = 0;
		virtual bool
			isCurrent( void ) = 0;

		static void
			registerShader( std::string name, std::filesystem::path, std::filesystem::path );
		static GLShaderProgram *
			getShader( std::string );

		static std::filesystem::path
			DefaultShaderPath;

		static std::shared_ptr<GLMesh>
			cube;

	protected:
		static long
			CurrentContext;
		GLContext( void ); // Abstract class, constructor disaled
		~GLContext( void );

	private:
		static std::map<std::string,GLShaderProgram>
			shaders;

	};
}

#endif
