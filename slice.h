#ifndef SLICE_H
#define SLICE_H

#include "global.h"

#include "rendering/renderable.h"

#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>

class vtkImageData;
class vtkImageReslice;

class Slice : public Renderable
{
public:
	Slice(vtkSmartPointer<vtkImageData> data);

	// (GL context)
	void bind();

	void setSlice(const Matrix4& mat, float clipDist, float zoomFactor);
	bool isEmpty() const { return mEmpty; }

	void setOpaque(bool opaque);

	// (GL context)
	void render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix);

private:
	// (GL context)
	void updateTexture();

	// (GL context)
	void switchMaterial(MaterialSharedPtr newMaterial);

	MaterialSharedPtr mMaterial;
	MaterialSharedPtr mDefaultMaterial, mOpaqueMaterial;
	GLuint mTextureHandle;
	vtkSmartPointer<vtkImageData> mData;
	Synchronized<vtkSmartPointer<vtkImageReslice>> mSliceFilter;
	vtkSmartPointer<vtkMatrix4x4> mTransformMatrix;
	bool mBound, mEmpty, mDirty, mOpaque;
	GLint mVertexAttrib, mTexCoordAttrib;
	GLint mProjectionUniform, mModelViewUniform;
	Synchronized<std::vector<unsigned char>> mTextureData;
	double mRange[2];
};

#endif /* SLICE_H */
