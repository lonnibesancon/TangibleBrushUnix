#ifndef LINES_H
#define LINES_H

#include "global.h"

#include "renderable.h"

class Lines : public Renderable
{
public:
	Lines();

	// (GL context)
	void bind();

	void setColor(const Vector3& color);
	void setOpacity(float opacity);

	// (GL context)
	void render(const Matrix4& projectionMatrix,
	            const Matrix4& modelViewMatrix);

	void setLines(const std::vector<Vector3>& lines);

private:
	MaterialSharedPtr mMaterial;
	bool mBound;
	GLint mVertexAttrib;
	GLint mProjectionUniform, mModelViewUniform, mColorUniform;
	Vector3 mColor;
	float mOpacity;
	Synchronized<std::vector<GLfloat>> mLineData;
};

#endif /* LINES_H */
