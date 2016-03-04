#ifndef ISOSURFACE_H
#define ISOSURFACE_H

#include "global.h"

#include <vtkSmartPointer.h>

#include "rendering/renderable.h"

class vtkImageData;

class IsoSurface : public Renderable
{
public:
	IsoSurface(vtkSmartPointer<vtkImageData> data, bool stream = false);

	// bind(true) must be called afterwards to update OpenGL buffers
	void setValue(double value);
	void setPercentage(double value);

	void setClipPlane(float a, float b, float c, float d); // plane equation: ax+by+cz+d=0
	void clearClipPlane();

	// (GL context)
	void bind();

	// (GL context)
	void render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix);

	bool isEmpty() const { return mIsEmpty; }

private:
	// (GL context)
	void update();

	MaterialSharedPtr mMaterial;
	vtkSmartPointer<vtkImageData> mData;
	double mValue, mPendingValue;
	bool mBound, mIsEmpty, mDirty, mStream;
	GLint mVertexAttrib, mNormalAttrib;
	GLint mProjectionUniform, mModelViewUniform, mNormalMatrixUniform, mDimensionsUniform, mValueUniform, mOpacityUniform, mClipPlaneUniform;
	GLuint mVertexBuffer, mNormalBuffer, mIndexBuffer;
	std::vector<GLfloat> mVertices, mNormals;
	std::vector<GLushort> mIndices;
	int mDimensions[3];
	double mRange[2];
	float mClipEq[4];
};

#endif /* ISOSURFACE_H */
