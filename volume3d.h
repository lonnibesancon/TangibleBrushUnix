#ifndef VOLUME3D_H
#define VOLUME3D_H

#include "global.h"

#include <vtkSmartPointer.h>

#include <list>

class vtkImageData;

class Volume3d
{
public:
	Volume3d(vtkSmartPointer<vtkImageData> data);
	~Volume3d();

	// (GL context)
	void bind();

	// (GL context)
	void render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix);

	void setClipPlane(float a, float b, float c, float d); // plane equation: ax+by+cz+d=0
	void clearClipPlane();

	double getMinValue() const { return mRange[0]; }
	double getMaxValue() const { return mRange[1]; }

private:
	bool hasClipPlane();

	MaterialSharedPtr mMaterial, mBackFaceMaterial;
	bool mBound;
	GLint mVertexAttrib, mBFVertexAttrib;
	GLint mProjectionUniform, mModelViewUniform, mDimensionsUniform, mClipPlaneUniform, mSpacingUniform, mScreenSizeUniform;
	GLint mBFProjectionUniform, mBFModelViewUniform, mBFDimensionsUniform, mBFSpacingUniform;
	GLuint mVertexBuffer;
	GLuint mIndexBuffer;
	std::vector<unsigned char> mTexture;
	int mDimensions[3];
	double mRange[2];
	Vector3 mSpacing;
	GLuint mTextureHandle, mBackFaceTextureHandle;
	GLuint mFrameBuffer;
	float mClipEq[4];
};

#endif /* VOLUME3D_H */
