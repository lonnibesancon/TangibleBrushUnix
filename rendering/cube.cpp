#include "cube.h"
#include "material.h"

namespace {
GLfloat vertices[] = { 1, 1, 1,  -1, 1, 1,  -1,-1, 1,      // v0-v1-v2 (front)
                       -1,-1, 1,   1,-1, 1,   1, 1, 1,      // v2-v3-v0

                        1, 1, 1,   1,-1, 1,   1,-1,-1,      // v0-v3-v4 (right)
                        1,-1,-1,   1, 1,-1,   1, 1, 1,      // v4-v5-v0

                        1, 1, 1,   1, 1,-1,  -1, 1,-1,      // v0-v5-v6 (top)
                       -1, 1,-1,  -1, 1, 1,   1, 1, 1,      // v6-v1-v0

                       -1, 1, 1,  -1, 1,-1,  -1,-1,-1,      // v1-v6-v7 (left)
                       -1,-1,-1,  -1,-1, 1,  -1, 1, 1,      // v7-v2-v1

                       -1,-1,-1,   1,-1,-1,   1,-1, 1,      // v7-v4-v3 (bottom)
                        1,-1, 1,  -1,-1, 1,  -1,-1,-1,      // v3-v2-v7

                        1,-1,-1,  -1,-1,-1,  -1, 1,-1,      // v4-v7-v6 (back)
                       -1, 1,-1,   1, 1,-1,   1,-1,-1 };    // v6-v5-v4

// normal array
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

	GLfloat wfvertices[] = {
         1.0f,  1.0f,  1.0f, // Vertex 0 (X, Y, Z)
        -1.0f,  1.0f,  1.0f, // Vertex 1 (X, Y, Z)

        -1.0f,  1.0f,  1.0f, // Vertex 1 (X, Y, Z)
        -1.0f, -1.0f,  1.0f, // Vertex 2 (X, Y, Z)

        -1.0f, -1.0f,  1.0f, // Vertex 2 (X, Y, Z)
         1.0f, -1.0f,  1.0f, // Vertex 3 (X, Y, Z)

         1.0f, -1.0f,  1.0f, // Vertex 3 (X, Y, Z)
         1.0f,  1.0f,  1.0f, // Vertex 0 (X, Y, Z)

         1.0f,  1.0f, -1.0f, // Vertex 4 (X, Y, Z)
        -1.0f,  1.0f, -1.0f, // Vertex 5 (X, Y, Z)

        -1.0f,  1.0f, -1.0f, // Vertex 5 (X, Y, Z)
        -1.0f, -1.0f, -1.0f, // Vertex 6 (X, Y, Z)

        -1.0f, -1.0f, -1.0f, // Vertex 6 (X, Y, Z)
         1.0f, -1.0f, -1.0f, // Vertex 7 (X, Y, Z)

         1.0f, -1.0f, -1.0f, // Vertex 7 (X, Y, Z)
         1.0f,  1.0f, -1.0f, // Vertex 4 (X, Y, Z)

         1.0f,  1.0f,  1.0f, // Vertex 0 (X, Y, Z)
         1.0f,  1.0f, -1.0f, // Vertex 4 (X, Y, Z)

        -1.0f,  1.0f,  1.0f, // Vertex 1 (X, Y, Z)
		-1.0f,  1.0f, -1.0f, // Vertex 5 (X, Y, Z)

		-1.0f, -1.0f,  1.0f, // Vertex 2 (X, Y, Z)
		-1.0f, -1.0f, -1.0f, // Vertex 6 (X, Y, Z)

		 1.0f, -1.0f,  1.0f, // Vertex 3 (X, Y, Z)
		 1.0f, -1.0f, -1.0f  // Vertex 7 (X, Y, Z)
    };

	const char* vertexShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform highp mat4 projection;\n"
		"uniform highp mat4 modelView;\n"
		"uniform mediump mat3 normalMatrix;\n"
		"attribute highp vec3 vertex;\n"
		"attribute mediump vec3 normal;\n"
		"varying mediump vec3 v_normal;\n"
		"void main() {\n"
		"  v_normal = normalize(normalMatrix * normal);\n"
		"  gl_Position = projection * modelView * vec4(vertex, 1.0);\n"
		"}";

	const char* fragmentShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform lowp vec4 color;\n"
		"varying mediump vec3 v_normal;\n"
		"void main() {\n"
		"  lowp vec3 lightDir = normalize(vec3(-1.0, 1.0, -0.5));\n" // FIXME: hardcoded light direction
		"  lowp float NdotL = max(dot(normalize(v_normal), lightDir), 0.0);\n"
		"  lowp float NdotL_rev = max(dot(-normalize(v_normal), lightDir), 0.0);\n" // backface lighting
		"  gl_FragColor = vec4(color.xyz*vec3(max(NdotL, NdotL_rev*0.5)), color.a);\n"
		"}";

	const char* wfVertexShader =
		// "#version 100\n"
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

	const char* wfFragmentShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform lowp vec4 color;\n"
		"void main() {\n"
		"  gl_FragColor = color;\n"
		"}";
} // namespace

Cube::Cube(bool wireframe)
 : mMaterial(MaterialSharedPtr(!wireframe ? new Material(vertexShader, fragmentShader) : new Material(wfVertexShader, wfFragmentShader))),
   mBound(false),
   mVertexAttrib(-1), mNormalAttrib(-1),
   mProjectionUniform(-1), mModelViewUniform(-1), mNormalMatrixUniform(-1), mColorUniform(-1),
   mColor(Vector3(0.5f)),
   mOpacity(1.0f),
   mWireframe(wireframe),
   mScale(Vector3::unit())
{}

void Cube::setColor(const Vector3& color)
{
	mColor = color;
}

void Cube::setOpacity(float opacity)
{
	mOpacity = opacity;
}

void Cube::setScale(const Vector3& scale)
{
	mScale = scale;
}

// (GL context)
void Cube::bind()
{
	mMaterial->bind();

	mVertexAttrib = mMaterial->getAttribute("vertex");
	mModelViewUniform = mMaterial->getUniform("modelView");
	mProjectionUniform = mMaterial->getUniform("projection");
	mColorUniform = mMaterial->getUniform("color");

	android_assert(mVertexAttrib != -1);
	android_assert(mModelViewUniform != -1);
	android_assert(mProjectionUniform != -1);
	android_assert(mColorUniform != -1);

	if (!mWireframe) {
		mNormalAttrib = mMaterial->getAttribute("normal");
		mNormalMatrixUniform = mMaterial->getUniform("normalMatrix");
		android_assert(mNormalAttrib != -1);
		android_assert(mNormalMatrixUniform != -1);
	}

	mBound = true;
}

// (GL context)
void Cube::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	if (!mBound)
		bind();

	glUseProgram(mMaterial->getHandle());

	if (!mWireframe) {
		// Vertices
		glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(mVertexAttrib);

		// Vertex normals
		glVertexAttribPointer(mNormalAttrib, 3, GL_FLOAT, false, 0, normals);

		glUniformMatrix3fv(mNormalMatrixUniform, 1, false, modelViewMatrix.inverse().transpose().get3x3Matrix().data_);
	} else {
		// Vertices
		glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false, 0, wfvertices);
		glEnableVertexAttribArray(mVertexAttrib);
	}

	// Uniforms
	glUniformMatrix4fv(mProjectionUniform, 1, false, projectionMatrix.data_);
	glUniformMatrix4fv(mModelViewUniform, 1, false, (modelViewMatrix * Matrix4(Matrix4::identity()).rescale(mScale)).data_);
	glUniform4f(mColorUniform, mColor.x, mColor.y, mColor.z, mOpacity);

	// Rendering
	// glDrawArrays(GL_TRIANGLES, 0, numPoints);
	if (!mWireframe) {
		glEnableVertexAttribArray(mNormalAttrib);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDisableVertexAttribArray(mNormalAttrib);
	} else {
		glDrawArrays(GL_LINES, 0, 24);
	}

	glDisableVertexAttribArray(mVertexAttrib);
}
