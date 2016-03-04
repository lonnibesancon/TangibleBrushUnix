#ifndef CUBE_H
#define CUBE_H

#include "global.h"

#include "renderable.h"

class Cube : public Renderable
{
public:
	Cube(bool wireframe = false);

	// (GL context)
	void bind();

	void setColor(const Vector3& color);
	void setOpacity(float opacity);
	void setScale(const Vector3& scale); // actually, this sets the *half* scale

	// (GL context)
	void render(const Matrix4& projectionMatrix,
	            const Matrix4& modelViewMatrix);

private:
	MaterialSharedPtr mMaterial;
	bool mBound;
	GLint mVertexAttrib, mNormalAttrib;
	GLint mProjectionUniform, mModelViewUniform, mNormalMatrixUniform, mColorUniform;
	Vector3 mColor;
	float mOpacity;
	bool mWireframe;
	Vector3 mScale;
};

#endif /* CUBE_H */
