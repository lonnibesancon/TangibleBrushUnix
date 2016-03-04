#include "lines.h"
#include "material.h"

namespace {
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

Lines::Lines()
 : mMaterial(MaterialSharedPtr(new Material(wfVertexShader, wfFragmentShader))),
   mBound(false),
   mVertexAttrib(-1),
   mProjectionUniform(-1), mModelViewUniform(-1), mColorUniform(-1),
   mColor(Vector3(0.5f)),
   mOpacity(1.0f)
{}

void Lines::setColor(const Vector3& color)
{
	mColor = color;
}

void Lines::setOpacity(float opacity)
{
	mOpacity = opacity;
}

// (GL context)
void Lines::bind()
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

	mBound = true;
}

void Lines::setLines(const std::vector<Vector3>& lines)
{
	android_assert(!(lines.size() % 2));

	synchronized (mLineData) {
		mLineData.clear();
		for (unsigned int i = 0; i < lines.size()/2; ++i) {
			const Vector3 pt1 = lines.at(i*2+0);
			const Vector3 pt2 = lines.at(i*2+1);
			mLineData.push_back(pt1.x);
			mLineData.push_back(pt1.y);
			mLineData.push_back(pt1.z);
			mLineData.push_back(pt2.x);
			mLineData.push_back(pt2.y);
			mLineData.push_back(pt2.z);
		}
	}
}

// (GL context)
void Lines::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	if (!mBound)
		bind();

	if (mLineData.empty())
		return;

	glUseProgram(mMaterial->getHandle());

	// Vertices
	glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false, 0, mLineData.data());
	glEnableVertexAttribArray(mVertexAttrib);

	// Uniforms
	glUniformMatrix4fv(mProjectionUniform, 1, false, projectionMatrix.data_);
	glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
	glUniform4f(mColorUniform, mColor.x, mColor.y, mColor.z, mOpacity);

	// Rendering
	glDrawArrays(GL_LINES, 0, mLineData.size()/3);

	glDisableVertexAttribArray(mVertexAttrib);
}
