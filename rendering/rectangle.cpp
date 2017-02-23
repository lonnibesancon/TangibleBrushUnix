#include "rendering/rectangle.h"
#include "material.h"

namespace
{
	GLfloat vertices[] = {1.0f, 1.0f, -1.0f,  0.0f, 1.0f, -1.0f,  0.0f,0.0f, -1.0f,      // v0-v1-v2 (front)
						   0.0f,0.0f, -1.0f,   1.0f,-0.0f, -1.0f,   1.0f, 1.0f, -1.0f};

	const char* vertexShader =
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform highp mat4 projection;\n"
		"uniform highp mat4 modelView;\n"
		"attribute highp vec3 vertex;\n"
		"void main() {\n"
		"  gl_Position = projection * modelView * vec4(vertex, 1.0);\n"
		"}";

	const char* fragmentShader =
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform mediump vec4 color;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = color;\n"
		"}\n";
} //namespace

Rectangle::Rectangle(float width, float height) : m_width(width), m_height(height), m_material(new Material(vertexShader, fragmentShader)),
	m_bound(false), m_vertexAttrib(-1), m_projectionUniform(-1), m_modelViewUniform(-1), m_colorUniform(-1),
	m_color(Vector3(1.0, 1.0, 1.0)), m_opacity(0.5f)
{}

void Rectangle::setColor(const Vector3& v)
{
	m_color = v;
}

void Rectangle::bind()
{
	m_material->bind();

	m_vertexAttrib = m_material->getAttribute("vertex");
	m_modelViewUniform = m_material->getUniform("modelView");
	m_projectionUniform = m_material->getUniform("projection");
	m_colorUniform = m_material->getUniform("color");

	android_assert(m_vertexAttrib != -1);
	android_assert(m_modelViewUniform != -1);
	android_assert(m_projectionUniform != -1);
	android_assert(m_colorUniform != -1);

	m_bound = true;
}

void Rectangle::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	if(!m_bound)
		bind();

	glUseProgram(m_material->getHandle());

	// Vertices
	glVertexAttribPointer(m_vertexAttrib, 3, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(m_vertexAttrib);

	// Uniforms
	glUniformMatrix4fv(m_projectionUniform, 1, false, projectionMatrix.data_);
	glUniformMatrix4fv(m_modelViewUniform, 1, false, (modelViewMatrix * Matrix4(Matrix4::identity()).rescale(Vector3(m_width, m_height, 1.0))).data_);
	glUniform4f(m_colorUniform, m_color.x, m_color.y, m_color.z, m_opacity);

	// Rendering
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(m_vertexAttrib);
	glUseProgram(0);
}

void Rectangle::setSize(float w, float h)
{
	m_width = w;
	m_height = h;
}
