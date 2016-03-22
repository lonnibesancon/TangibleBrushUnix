#include "slice.h"

#include "rendering/material.h"

#include <limits>
#include <cmath>

#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkFloatArray.h>
#include <vtkTransform.h>

namespace {
	const GLfloat texturedQuadBuffer[] = {
	    //X, Y, Z,  U,  V
		-1, -1, 0,  0, 1,  // bottom left
		 1, -1, 0,  1, 1,  // bottom right
		-1,  1, 0,  0, 0,  // top left
		 1,  1, 0,  1, 0   // top right
	};

	const GLsizei bufferTypeSize = sizeof(texturedQuadBuffer[0]);
	const GLsizei bufferStride = 5 * bufferTypeSize;
	const GLsizei numPoints = sizeof(texturedQuadBuffer)/bufferStride;

	// const unsigned int horizSize = 128, vertSize = 128;
	const unsigned int horizSize = 256*4, vertSize = 256*4;
	// const unsigned int horizSize = 1280, vertSize = 736; // screenWidth/screenHeight

	const char* vertexShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform highp mat4 projection;\n"
		"uniform highp mat4 modelView;\n"
		"attribute highp vec3 vertex;\n"
		"attribute mediump vec2 texCoord;\n"
		"varying mediump vec2 v_texCoord;\n"
		"void main() {\n"
		"  v_texCoord = texCoord;\n"
		"  gl_Position = projection * modelView * vec4(vertex, 1.0);\n"
		"}";

	const char* fragmentShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"varying mediump vec2 v_texCoord;\n"
		"uniform highp sampler2D texture;\n"
		// "Jet" color map (http://www.metastine.com/?p=7)
		// FIXME: duplicated code (see isosurface.cpp and volume.cpp)
		"lowp vec3 colormap(lowp float value) {\n"
		"  lowp float value4 = 4.0 * value;\n"
		"  lowp float a =  value4 - 1.5;\n"
		"  lowp float b = -value4 + 4.5;\n"
		"  lowp float c =  value4 - 0.5;\n"
		"  lowp float d = -value4 + 3.5;\n"
		"  lowp float e =  value4 + 0.5;\n"
		"  lowp float f = -value4 + 2.5;\n"
		"  return clamp(vec3(min(a,b), min(c,d), min(e,f)), 0.0, 1.0);\n"
		"}\n"
		"void main() {\n"
		"  lowp vec4 values = texture2D(texture, v_texCoord);\n"
		"  if (values.a > 0.0) gl_FragColor = vec4(colormap(values.r), 1.0); else discard;\n"
		"}";

	const char* fragmentShader2 =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"varying mediump vec2 v_texCoord;\n"
		"uniform highp sampler2D texture;\n"
		// "Jet" color map (http://www.metastine.com/?p=7)
		// FIXME: duplicated code (see isosurface.cpp and volume.cpp)
		"lowp vec3 colormap(lowp float value) {\n"
		"  lowp float value4 = 4.0 * value;\n"
		"  lowp float a =  value4 - 1.5;\n"
		"  lowp float b = -value4 + 4.5;\n"
		"  lowp float c =  value4 - 0.5;\n"
		"  lowp float d = -value4 + 3.5;\n"
		"  lowp float e =  value4 + 0.5;\n"
		"  lowp float f = -value4 + 2.5;\n"
		"  return clamp(vec3(min(a,b), min(c,d), min(e,f)), 0.0, 1.0);\n"
		"}\n"
		"void main() {\n"
		"  lowp vec4 values = texture2D(texture, v_texCoord);\n"
		// "  if (values.a > 0.0) gl_FragColor = vec4(colormap(values.r), 1.0); else discard;\n"
		"  if (values.a > 0.0) gl_FragColor = vec4(colormap(values.r), 1.0); else gl_FragColor = vec4(vec3(0.5), 0.5);\n" // XXX: debug
		"}";
} // namespace

Slice::Slice(vtkSmartPointer<vtkImageData> data)
 : mDefaultMaterial(MaterialSharedPtr(new Material(vertexShader, fragmentShader))),
   mOpaqueMaterial(MaterialSharedPtr(new Material(vertexShader, fragmentShader2))),
   mTextureHandle(0),
   mData(data),
   mSliceFilter(vtkSmartPointer<vtkImageReslice>::New()),
   mTransformMatrix(vtkMatrix4x4::New()),
   mBound(false), mEmpty(true), mDirty(false), mOpaque(false),
   mVertexAttrib(-1), mTexCoordAttrib(-1),
   mProjectionUniform(-1), mModelViewUniform(-1)
{
	android_assert(mData);
	android_assert(mSliceFilter);

	mData->GetPointData()->GetScalars()->GetRange(mRange);
	LOGD("range min=%f max=%f", mRange[0], mRange[1]);

	mSliceFilter->SetInputData(mData);
	mSliceFilter->SetInterpolationModeToLinear();
	mSliceFilter->SetOutputDimensionality(2);
	mSliceFilter->BorderOn();

	// Set the slice viewport to a horizSize x vertSize area.
	// mSliceFilter->SetResliceAxes() will then transform this
	// viewport according to the given transformation matrix, and
	// slice through the volume along the transformed viewport.
	mSliceFilter->SetOutputExtent(0, horizSize-1, 0, vertSize-1, 0, 0);

	mSliceFilter->TransformInputSamplingOff(); // NOTE: important!!!!

	// Mask for pixels outside of the data slice
	mSliceFilter->SetBackgroundLevel(std::numeric_limits<double>::infinity());

	mTextureData.resize(2*horizSize*vertSize);
}

// (GL context)
void Slice::switchMaterial(MaterialSharedPtr newMaterial)
{
	if (mMaterial == newMaterial)
		return;

	android_assert(newMaterial);
	mMaterial = newMaterial;

	mMaterial->bind();

	mVertexAttrib = mMaterial->getAttribute("vertex");
	mTexCoordAttrib = mMaterial->getAttribute("texCoord");
	mProjectionUniform = mMaterial->getUniform("projection");
	mModelViewUniform = mMaterial->getUniform("modelView");

	android_assert(mVertexAttrib != -1);
	android_assert(mTexCoordAttrib != -1);
	android_assert(mProjectionUniform != -1);
	android_assert(mModelViewUniform != -1);
}

// (GL context)
void Slice::bind()
{
	if (mMaterial)
		mMaterial->bind();

	glGenTextures(1, &mTextureHandle);

	glBindTexture(GL_TEXTURE_2D, mTextureHandle);

	// Only GL_NEAREST/GL_LINEAR and GL_CLAMP_TO_EDGE are
	// supported for non-power-of-two textures according to the
	// OpenGL ES 2.0 specification
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Initialize the texture with no content
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_LUMINANCE_ALPHA,
		horizSize, vertSize,
		0,
		GL_LUMINANCE_ALPHA,
		GL_UNSIGNED_BYTE,
		nullptr
	);

	mBound = true;
}

void Slice::setSlice(const Matrix4& mat, float clipDist, float zoomFactor)
{
	// TODO: an early test to check if the slice would be empty

	synchronized(mSliceFilter) {
		double m[16];
		for (int i = 0; i < 16; ++i)
			m[i] = mat.data_[i];
		mTransformMatrix->DeepCopy(m);

		// "mat" is column-major, but vtkMatrix4x4 is row-major
		mTransformMatrix->Transpose();

		mSliceFilter->SetResliceAxes(mTransformMatrix);

		// FIXME: why *0.5 ?
		mSliceFilter->SetOutputSpacing(clipDist/(horizSize*zoomFactor*0.5), clipDist/(vertSize*zoomFactor*0.5), 1);

		mSliceFilter->Update();

		vtkImageData* image = mSliceFilter->GetOutput();
		android_assert(image);

#ifndef NDEBUG
		int dimensions[3];
		image->GetDimensions(dimensions);
		android_assert(dimensions[0] == static_cast<signed>(horizSize));
		android_assert(dimensions[1] == static_cast<signed>(vertSize));
		android_assert(dimensions[2] == 1);
#endif

		// unsigned char* data = static_cast<unsigned char*>(image->GetScalarPointer());
		// android_assert(data);

		vtkDataArray* scalars = image->GetPointData()->GetScalars();
		android_assert(scalars);

		unsigned int num = scalars->GetNumberOfTuples();
		android_assert(num == static_cast<unsigned>(dimensions[0]*dimensions[1]));

		// vtkFloatArray* arr = vtkFloatArray::SafeDownCast(scalars);
		// android_assert(arr);

		// const float* ptr = arr->GetPointer(0);
		// const std::size_t N = scalars->GetNumberOfComponents();

		bool notEmpty = false;
		synchronized(mTextureData) {
			for (unsigned int i = 0; i < num; ++i) {
				double value = scalars->GetComponent(i, 0);
				// float value = *ptr;
				// ptr += N;
				// FIXME: hardcoded constant (signed int16 max, see head.vti)
				bool isinf = (__isinf(value) || value == 32767 || value == 255);
				mTextureData[i*2+1] = (isinf ? 0 : 255);
				if (!isinf) {
					notEmpty = true;
					double norm = (value-mRange[0]) / (mRange[1]-mRange[0]);
					mTextureData[i*2+0] = norm*255;
				}
				// if (!mInitialized) LOGD("%d => %f", i, value);
			}
		}
		mEmpty = !notEmpty;
	}

	mDirty = true;
	// mInitialized = true;
}

void Slice::setOpaque(bool opaque)
{
	mOpaque = opaque;
}

// (GL context)
void Slice::updateTexture()
{
	android_assert(mTextureHandle != 0);

	glBindTexture(GL_TEXTURE_2D, mTextureHandle);

	// Required because input is not RGBA (i.e. not aligned to a 4-byte boundary)
	// http://www.opengl.org/wiki/Common_Mistakes#Texture_upload_and_pixel_reads
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	synchronized(mTextureData) {
		// Update the texture contents
		glTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			0, 0,
			horizSize, vertSize,
			GL_LUMINANCE_ALPHA,
			GL_UNSIGNED_BYTE,
			mTextureData.data()
		);
	}
}

// (GL context)
void Slice::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	if (mEmpty && !mOpaque)
		return;

	if (!mBound)
		bind();

	if (mDirty) {
		updateTexture();
		mDirty = false;
	}

	switchMaterial(mOpaque ? mOpaqueMaterial : mDefaultMaterial);
	android_assert(mMaterial);

	// Vertices
	glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false,
		bufferStride, texturedQuadBuffer);
	glEnableVertexAttribArray(mVertexAttrib);

	// Texture coordinates
	glVertexAttribPointer(mTexCoordAttrib, 2, GL_FLOAT, false,
		bufferStride, &texturedQuadBuffer[3]);
	glEnableVertexAttribArray(mTexCoordAttrib);

	// Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTextureHandle);

	// Uniforms
	glUseProgram(mMaterial->getHandle());
	glUniformMatrix4fv(mProjectionUniform, 1, false, projectionMatrix.data_);
	glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);

	// Rendering
	glDrawArrays(GL_TRIANGLE_STRIP, 0, numPoints);

	glDisableVertexAttribArray(mVertexAttrib);
	glDisableVertexAttribArray(mTexCoordAttrib);
}
