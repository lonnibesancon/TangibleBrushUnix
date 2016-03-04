#ifndef MATERIAL_H
#define MATERIAL_H

#include "global.h"

class Material
{
public:
	Material(const std::string& vertexShaderSrc,
	         const std::string& fragmentShaderSrc);

	GLuint getHandle() const { return mProgramHandle; }

	// (GL context)
	void bind();

	// (GL context)
	GLint getAttribute(const std::string& name) const;
	// (GL context)
	GLint getUniform(const std::string& name) const;

private:
	// (GL context)
	static GLuint compileProgram(
		const std::string& vertexShaderSrc,
		const std::string& fragmentShaderSrc);

	// (GL context)
	static GLuint compileShader(GLenum type, const std::string& source);

	GLuint mProgramHandle;
	std::string mVertexShaderSrc, mFragmentShaderSrc;
};

#endif /* MATERIAL_H */
