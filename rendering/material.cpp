#include "material.h"

namespace {
	void checkError(const char* msg)
	{
		GLenum err;
		while ((err = glGetError()) != 0) {
			LOGD("%s: error = %d", msg, err);
		}
	}
	#define CHECK(x) (x); checkError(#x);
	// #define CHECK(x) (x)

} // namespace

Material::Material(const std::string& vertexShaderSrc,
                   const std::string& fragmentShaderSrc)
 : mProgramHandle(0),
   mVertexShaderSrc(vertexShaderSrc), mFragmentShaderSrc(fragmentShaderSrc)
{}

// (GL context)
void Material::bind()
{
	mProgramHandle = compileProgram(mVertexShaderSrc, mFragmentShaderSrc);
}

// (GL context)
GLint Material::getAttribute(const std::string& name) const
{
	android_assert(mProgramHandle != 0);
	return glGetAttribLocation(mProgramHandle, name.c_str());
}

// (GL context)
GLint Material::getUniform(const std::string& name) const
{
	android_assert(mProgramHandle != 0);
	return glGetUniformLocation(mProgramHandle, name.c_str());
}

// (GL context)
GLuint Material::compileProgram(
	const std::string& vertexShaderSrc,
	const std::string& fragmentShaderSrc)
{
	GLuint vertexShader = 0, fragmentShader = 0;

	// LOGD("compileVShader(%s)", vertexShaderSrc.c_str());
	if ((vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSrc)) == 0)
		return 0;

	// LOGD("compileFShader(%s)", fragmentShaderSrc.c_str());
	if ((fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc)) == 0)
		return 0;

	GLuint program = CHECK(glCreateProgram());
	if (program != 0) {
		CHECK(glAttachShader(program, vertexShader));
		CHECK(glAttachShader(program, fragmentShader));
		CHECK(glLinkProgram(program));
		GLint linkStatus;
		CHECK(glGetProgramiv(program, GL_LINK_STATUS, &linkStatus));
		if (linkStatus != GL_TRUE) {
			LOGE("Could not link program:\n"
			     "---- vertex shader ----\n%s\n"
			     "---- fragment shader ---\n%s",
			     vertexShaderSrc.c_str(),
			     fragmentShaderSrc.c_str());
			int maxLength;
			int infologLength = 0;
			CHECK(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength));
			std::vector<char> infoLog(maxLength);
			CHECK(glGetProgramInfoLog(program, maxLength, &infologLength, infoLog.data()));
			if (infologLength > 0)
				LOGE("%s", infoLog.data());
			CHECK(glDeleteProgram(program));
			program = 0;
		}
	}

	return program;
}

// (GL context)
GLuint Material::compileShader(GLenum type, const std::string& source)
{
	GLuint shader = CHECK(glCreateShader(type));
	if (shader != 0) {
		const char* src = source.c_str();
		CHECK(glShaderSource(shader, 1, &src, nullptr));
		CHECK(glCompileShader(shader));
		GLint compiled;
		CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled));
		if (compiled == 0) {
			LOGE("Could not compile %s shader:", (type == GL_VERTEX_SHADER ? "vertex" : "fragment"));
			LOGE("%s", src);
			int maxLength;
			int infologLength = 0;
			CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength));
			std::vector<char> infoLog(maxLength);
			CHECK(glGetShaderInfoLog(shader, maxLength, &infologLength, infoLog.data()));
			if (infologLength > 0)
				LOGE("%s", infoLog.data());
			CHECK(glDeleteShader(shader));
			shader = 0;
		}
	}

	return shader;
}
