#ifndef VOLUME_H
#define VOLUME_H

#include "global.h"

#include <vtkSmartPointer.h>

#include <list>

class vtkImageData;

class Volume
{
public:
	Volume(vtkSmartPointer<vtkImageData> data);
	~Volume();

	// (GL context)
	void bind();

	// (GL context)
	void render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix);

	void setClipPlane(float a, float b, float c, float d); // plane equation: ax+by+cz+d=0
	void clearClipPlane();

	double getMinValue() const { return mRange[0]; }
	double getMaxValue() const { return mRange[1]; }

	void setOpacity(float opacity) { mOpacity = opacity; }

private:
	bool hasClipPlane();

	// (GL context)
	void initXPlanes(unsigned int& baseIndex);
	void initYPlanes(unsigned int& baseIndex);
	void initZPlanes(unsigned int& baseIndex);

	// (GL context)
	void switchMaterial(MaterialSharedPtr newMaterial);

	MaterialSharedPtr mMaterial;
	MaterialSharedPtr mMaterialClip, mMaterialFast;
	bool mBound;
	GLint mVertexAttrib, mTexCoordAttrib, mSliceAttrib;
	GLint mProjectionUniform, mModelViewUniform, mDimensionsUniform, mInvertUniform, mClipPlaneUniform, mSpacingUniform, mOpacityUniform;
	GLuint mVertexBuffer, mTexCoordBuffer;
	GLuint mIndexBufferX, mIndexBufferY, mIndexBufferZ;
	std::vector<GLfloat> mVertices, mTexCoords;
	std::vector<GLushort> mIndicesX, mIndicesY, mIndicesZ;
	// std::list<std::vector<unsigned char>> mTexturesX, mTexturesY, mTexturesZ;
	std::vector<unsigned char> mTexture;
	int mDimensions[3];
	double mRange[2];
	Vector3 mSpacing;
	// GLuint mTextureXHandle, mTextureYHandle, mTextureZHandle;
	GLuint mTextureHandle;
	float mClipEq[4];
	float mOpacity;
};

#endif /* VOLUME_H */
