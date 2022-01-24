
#include "GLShaderProgram.class.hpp"

#include <iostream>

namespace notrealengine
{
	GLShaderProgram::GLShaderProgram( const ShaderPart<GL_VERTEX_SHADER>& vertex,
									  const ShaderPart<GL_FRAGMENT_SHADER>& fragment ):
		programID(GLCallThrow(glCreateProgram))
	{
		try {
			GLCallThrow(glAttachShader, programID, vertex.shaderID);
			GLCallThrow(glAttachShader, programID, fragment.shaderID);
			GLCallThrow(glLinkProgram, programID);
			int	success = 0;
			char	infoLog[512];
			GLCallThrow(glGetProgramiv, programID, GL_LINK_STATUS, &success);
			if (success == 0)
			{
				GLCallThrow(glGetProgramInfoLog, programID, 512, NULL, infoLog);
				std::cerr << "Failed to link shader:" << infoLog << std::endl;
			}
		} catch (std::exception e) {
			GLCallThrow(glDeleteProgram, programID);
		}
	}

	GLShaderProgram::~GLShaderProgram( )
	{
		try {
			GLCallThrow(glDeleteProgram, (programID));
			int	success = 0;
			char	infoLog[512];
			GLCallThrow(glGetProgramiv, programID, GL_DELETE_STATUS, &success);
			if (success == 0)
			{
				GLCallThrow(glGetProgramInfoLog, programID, 512, NULL, infoLog);
				std::cerr << "Failed to delete shader:" << infoLog << std::endl;
			}
		} catch (std::exception e) {
			std::cerr << "Exception caught in destructor (!) :" << std::endl << e.what();
		}
	}

	template<GLenum Type>
	GLShaderProgram::ShaderPart<Type>::ShaderPart( const std::string& code ):
		shaderID(GLCallThrow(glCreateShader, Type))
	{
		try {
			const char* str = code.c_str();
			GLCallThrow(glShaderSource, shaderID, 1, &str, NULL);
			GLCallThrow(glCompileShader, shaderID);
			int	success = 0;
			char	infoLog[512];
			GLCallThrow(glGetShaderiv, shaderID, GL_COMPILE_STATUS, &success);
			if (success == 0)
			{
				GLCallThrow(glGetShaderInfoLog, shaderID, 512, NULL, infoLog);
				std::cerr << "Failed to compile shader:" << infoLog << std::endl;
			}
		} catch (std::exception e) {
			GLCallThrow(glDeleteShader, shaderID);
		}
	}

	template<GLenum Type>
	GLShaderProgram::ShaderPart<Type>::~ShaderPart( )
	{
		try {
			GLCallThrow(glDeleteShader, shaderID);
			int	success = 0;
			char	infoLog[512];
			GLCallThrow(glGetShaderiv, shaderID, GL_DELETE_STATUS, &success);
			if (success == 0)
			{
				GLCallThrow(glGetShaderInfoLog, shaderID, 512, NULL, infoLog);
				std::cerr << "Failed to delete shader:" << infoLog << std::endl;
			}
		} catch (std::exception e) {
			std::cerr << "Exception caught in destructor (!) :" << std::endl << e.what();
		}
	}

	template class GLShaderProgram::ShaderPart<GL_VERTEX_SHADER>;
	template class GLShaderProgram::ShaderPart<GL_FRAGMENT_SHADER>;
} // namespace notrealengine
