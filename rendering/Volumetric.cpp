#include "rendering/Volumetric.h"
#include "material.h"

namespace
{
	GLfloat vertices[] = { 1, 1, 1,  0, 1, 1,  0,0, 1,      // v0-v1-v2 (front)
                       0,0, 1,   1,0, 1,   1, 1, 1,      // v2-v3-v0

                        1, 1, 1,   1,0, 1,   1,0,0,      // v0-v3-v4 (right)
                        1,0,0,   1, 1,0,   1, 1, 1,      // v4-v5-v0

                        1, 1, 1,   1, 1,0,  0, 1,0,      // v0-v5-v6 (top)
                       0, 1,0,  0, 1, 1,   1, 1, 1,      // v6-v1-v0

                       0, 1, 1,  0, 1,0,  0,0,0,      // v1-v6-v7 (left)
                       0,0,0,  0,0, 1,  0, 1, 1,      // v7-v2-v1

                       0,0,0,   1,0,0,   1,0, 1,      // v7-v4-v3 (bottom)
                        1,0, 1,  0,0, 1,  0,0,0,      // v3-v2-v7

                        1,0,0,  0,0,0,  0, 1,0,      // v4-v7-v6 (back)
                       0, 1,0,   1, 1,0,   1,0,0 };    // v6-v5-v4

	GLfloat normals[]  = { 0, 0, 1,   0, 0, 1,   0, 0, 1,      // v0-v1-v2 (front)
                        0, 0, 1,   0, 0, 1,   0, 0, 1,      // v2-v3-v0

                        1, 0, 0,   1, 0, 0,   1, 0, 0,      // v0-v3-v4 (right)
                        1, 0, 0,   1, 0, 0,   1, 0, 0,      // v4-v5-v0

                        0, 1, 0,   0, 1, 0,   0, 1, 0,      // v0-v5-v6 (top)
                        0, 1, 0,   0, 1, 0,   0, 1, 0,      // v6-v1-v0

                       -1, 0, 0,  -1, 0, 0,  -1, 0, 0,      // v1-v6-v7 (left)
                       -1, 0, 0,  -1, 0, 0,  -1, 0, 0,      // v7-v2-v1

                        0,-1, 0,   0,-1, 0,   0,-1, 0,      // v7-v4-v3 (bottom)
                        0,-1, 0,   0,-1, 0,   0,-1, 0,      // v3-v2-v7

                        0, 0,-1,   0, 0,-1,   0, 0,-1,      // v4-v7-v6 (back)
                        0, 0,-1,   0, 0,-1,   0, 0,-1 };    // v6-v5-v4

	const char* vertexShader = 
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"
		"uniform highp mat4 projection;\n"
		"uniform highp mat4 modelView;\n"
		"attribute highp vec3 vertex;\n"
		"attribute mediump vec3 normal;\n"

		"varying mediump vec3 vPos;\n"
		"void main() {\n"
		"  vPos = vertex;\n"
		"  gl_Position = projection * modelView * vec4(vertex, 1.0);\n"
		"}";

	const char* fragmentShader =
		"#version 130\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform sampler3D volume;\n"
		"varying mediump vec3 vPos;\n"

		"void main() {\n"
		"  gl_FragColor = texture(volume, vPos);\n"
		"}";
}

Volumetric::Volumetric(FillVolume* fill, const Vector3_f& c, float alpha) :
	m_material(MaterialSharedPtr(new Material(vertexShader, fragmentShader))),
   	m_bound(false),
    m_vertexAttrib(-1), m_normalAttrib(-1),
    m_projectionUniform(-1), m_modelViewUniform(-1), m_normalMatrixUniform(-1), m_volumeUniform(-1)
{
	glGenTextures(1, &m_textureId);
	uint64_t bufferSize = fill->getMetricsSizeX()*fill->getMetricsSizeY()*fill->getMetricsSizeZ();
    //RGBA buffer
    float* chRGBABuffer = new float[bufferSize*4];

	for(uint32_t i=0; i < bufferSize; i++)
	{
		bool v = fill->get(i);
		chRGBABuffer[i*4] = c.x;
		chRGBABuffer[i*4+1] = c.y;
		chRGBABuffer[i*4+2] = c.z;
		chRGBABuffer[i*4+3] = (v) ? 0.5: 0.0;
	}


	glBindTexture(GL_TEXTURE_3D, m_textureId);
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA,
			fill->getMetricsSizeX(), fill->getMetricsSizeY(), fill->getMetricsSizeZ(),
		   	0, GL_RGBA, GL_FLOAT, (GLvoid *) chRGBABuffer);
	}
	glBindTexture(GL_TEXTURE_3D, 0);
	delete[] chRGBABuffer;
}

Volumetric::~Volumetric()
{
	glDeleteTextures(1, &m_textureId);
}

void Volumetric::bind()
{
	m_material->bind();

	m_vertexAttrib = m_material->getAttribute("vertex");
	m_modelViewUniform = m_material->getUniform("modelView");
	m_projectionUniform = m_material->getUniform("projection");
	m_volumeUniform = m_material->getUniform("volume");

	m_bound = true;
}

void Volumetric::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	if(!m_bound)
		bind();

	glUseProgram(m_material->getHandle());
	{
		glVertexAttribPointer(m_vertexAttrib, 3, GL_FLOAT, false, 0, vertices);

		// Vertex normals
		glVertexAttribPointer(m_normalAttrib, 3, GL_FLOAT, false, 0, normals);
		glUniformMatrix4fv(m_projectionUniform, 1, false, projectionMatrix.data_);
		glUniformMatrix4fv(m_modelViewUniform, 1, false, (modelViewMatrix * Matrix4(Matrix4::identity())).data_);

		glEnableVertexAttribArray(m_vertexAttrib);
		glEnableVertexAttribArray(m_normalAttrib);
		glBindTexture(GL_TEXTURE_3D, m_textureId);
			glUniform1i(m_volumeUniform, 0); 
			glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindTexture(GL_TEXTURE_3D, 0);
		glDisableVertexAttribArray(m_normalAttrib);
		glDisableVertexAttribArray(m_vertexAttrib);
	}
	glUseProgram(0);
}
