#include "volume.h"

#include "rendering/material.h"
#include "getprocaddress.h"

#include <limits>

#include <vtkNew.h>
#include <vtkDataSetReader.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkExtractVOI.h>
#include <vtkDataArray.h>

namespace {
	// Sampling step (plane spacing)
	const unsigned int step = 1;
	// const unsigned int step = 2;
	// const unsigned int step = 3;

	/* NOTE: interesting trick:

	  #define STR(...) #__VA_ARGS__
	  auto code = STR(
	     // unquoted shader code
	  );

	  Drawbacks: does not preserve newlines or extra whitespace, and does
	  not allow unbalanced expressions (this may actually be useful!) or
	  GLSL preprocessor instructions (such as #version ...).
	 */
	const char* vertexShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform highp mat4 projection;\n"
		"uniform highp mat4 modelView;\n"
		"uniform lowp ivec3 dimensions;\n" // (dimX,dimY,dimZ)
		"uniform lowp vec3 spacing;\n"
		"uniform lowp vec3 invert;\n" // != 0 when planes are flipped
		"attribute highp vec3 vertex;\n"
		"attribute mediump vec3 texCoord;\n"
		"varying mediump vec3 v_texCoord;\n"
		"uniform lowp bool selectionMode;\n"

		"uniform highp vec4 clipPlane;\n"
		"varying highp float v_clipDist;\n"
		"varying highp vec4 v_posNotModified;\n"
		"uniform highp mat4 postTreatment;\n"

		"void main() {\n"
		// "  if (invert == 0)\n"
		// "    v_texCoord = texCoord;\n"
		// "  else\n"
		// "    v_texCoord = vec3(texCoord.xy, float(dimensions.w-1)-texCoord.z);\n"
		"  v_texCoord = mix(texCoord, 1.0-texCoord, invert);\n"
		// "  mediump vec3 scale = vec3(float(dimensions.x), float(dimensions.y), float(dimensions.z));\n"
		"  mediump vec3 scale = vec3(float(dimensions.x), float(dimensions.y), float(dimensions.z)) * spacing;\n"
		"  highp vec4 viewSpacePos = modelView * vec4(scale * (vertex * vec3(1.0, 1.0, -1.0)), 1.0);\n"

		"  if(selectionMode) gl_Position = projection * postTreatment * vec4(scale * (vertex * vec3(1.0, 1.0, -1.0)), 1.0);\n"
		"  else              gl_Position = projection * viewSpacePos;\n"

		"  v_clipDist = dot(viewSpacePos.xyz, clipPlane.xyz) + clipPlane.w;\n"
		"  v_posNotModified = viewSpacePos;\n"
		"}";

	// // #ifndef GL_TEXTURE_2D_ARRAY_EXT
	// // 	#define GL_TEXTURE_2D_ARRAY_EXT 0x8C1A
	// // #endif
	//
	// #ifndef GL_TEXTURE_3D_OES
	// 	#define GL_TEXTURE_3D_OES 0x806F
	// #endif
	//
	// #ifndef GL_TEXTURE_WRAP_R_OES
	// 	#define GL_TEXTURE_WRAP_R_OES 0x8072
	// #endif

	const char* fragmentShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		// "#extension GL_EXT_texture_array : require\n"
		"//#extension GL_OES_texture_3D : require\n"
		"varying mediump vec3 v_texCoord;\n"
		// "uniform lowp sampler2DArray texture;\n"
		"uniform lowp sampler3D texture;\n"
		"uniform lowp float opacity;\n"

		"uniform highp mat4 projection;\n"
		"uniform highp vec3 uFirstPos;\n"
		"uniform highp vec3 uLastPos;\n"
		"uniform highp mat4 selectionMat;\n"
		"uniform lowp bool selectionMode;\n"
		"uniform highp mat4 modelView;\n"
		"uniform highp mat4 postTreatment;\n"
		"varying highp vec4 v_posNotModified;\n"

		"varying highp float v_clipDist;\n"

		// // "Jet" color map (http://www.metastine.com/?p=7)
		// // FIXME: duplicated code (see isosurface.cpp)
		// "lowp vec3 colormap(lowp float value) {\n"
		// "  lowp float value4 = 4.0 * value;\n"
		// "  lowp float a =  value4 - 1.5;\n"
		// "  lowp float b = -value4 + 4.5;\n"
		// "  lowp float c =  value4 - 0.5;\n"
		// "  lowp float d = -value4 + 3.5;\n"
		// "  lowp float e =  value4 + 0.5;\n"
		// "  lowp float f = -value4 + 2.5;\n"
		// "  return clamp(vec3(min(a,b), min(c,d), min(e,f)), 0.0, 1.0);\n"
		// "}\n"
		"void main() {\n"
		"if (v_clipDist <= 50.0) discard;\n"
		//Selection !
		"  if(selectionMode)\n"
		"  {\n"
		"     vec4 testPos = selectionMat * v_posNotModified;\n"
		//"     vec4 testPos = v_pos; \n"
		
		"     vec4 fp = (vec4(uFirstPos.x, uFirstPos.y, 0.0f, 1.0));\n"
		"     fp = testPos.w * fp;\n"
		"     vec4 lp = (vec4(uLastPos.x, uLastPos.y, 0.05f, 1.0));\n"
		"     lp = testPos.w * lp;\n"

		"     float fx = min(fp.x, lp.x);\n"
		"     float fy = min(fp.y, lp.y);\n"
		"     float fz = min(fp.z, lp.z);\n"

		"     float lx = max(fp.x, lp.x);\n"
		"     float ly = max(fp.y, lp.y);\n"
		"     float lz = max(fp.z, lp.z);\n"
	    
	    "	  if(testPos.x < fx || testPos.x > lx || testPos.y < fy || testPos.y > ly || testPos.z < fz || testPos.z > lz) discard; \n"
		"  }\n"

		// "  lowp float value = texture2DArray(texture, v_texCoord).a;\n"
		// "  gl_FragColor = vec4(colormap(value), 0.03+0.3*value);\n"
		// "  gl_FragColor = vec4(vec3(value), 0.03+0.3*value);\n"
		// "  gl_FragColor = vec4(colormap(value), 1.0);\n"

		// "  gl_FragColor = texture2DArray(texture, v_texCoord);\n"
		"  gl_FragColor = texture3D(texture, v_texCoord) * vec4(1.0, 1.0, 1.0, opacity);\n"

		"}";

	void colormap(double value, float& r, float& g, float& b_) {
		float value4 = 4.0f * value;
		float a =  value4 - 1.5f;
		float b = -value4 + 4.5f;
		float c =  value4 - 0.5f;
		float d = -value4 + 3.5f;
		float e =  value4 + 0.5f;
		float f = -value4 + 2.5f;
		r  = std::min(std::max(std::min(a,b), 0.0f), 1.0f);
		g  = std::min(std::max(std::min(c,d), 0.0f), 1.0f);
		b_ = std::min(std::max(std::min(e,f), 0.0f), 1.0f);
	}

	Synchronized<std::list<GLuint>> staleTexturesList;

} // namespace

Volume::Volume(vtkSmartPointer<vtkImageData> data)
 : mMaterialClip(MaterialSharedPtr(new Material(vertexShader, /*"#version 100\n" +*/ std::string(fragmentShader)))),
   mMaterialFast(MaterialSharedPtr(new Material(vertexShader, /*"#version 100\n*/"#define FAST\n" + std::string(fragmentShader)))),
   mBound(false),
   mVertexAttrib(-1), mTexCoordAttrib(-1),
   mProjectionUniform(-1), mModelViewUniform(-1), mDimensionsUniform(-1), mInvertUniform(-1), mClipPlaneUniform(-1), mSpacingUniform(-1), mOpacityUniform(-1),
   // mTextureXHandle(0), mTextureYHandle(0), mTextureZHandle(0)
   mTextureHandle(0),
   mOpacity(1.0f)
{
	android_assert(data);

	clearClipPlane();

	// Check if the data dimension is 3
	int dim = data->GetDataDimension();
	LOGD("dimension = %d", dim);
	if (dim != 3) {
		throw std::runtime_error(
			"Volume: data is not 3D (dimension = " + Utility::toString(dim) + ")"
		);
	}

	if (!data->GetPointData() || !data->GetPointData()->GetScalars())
		throw std::runtime_error("IsoSurface: unsupported data");

	data->GetDimensions(mDimensions);
	LOGD("dimensions %d %d %d", mDimensions[0], mDimensions[1], mDimensions[2]);

	double spacing[3];
	data->GetSpacing(spacing);
	LOGD("spacing %f %f %f", spacing[0], spacing[1], spacing[2]);
	mSpacing = Vector3(spacing[0], spacing[1], spacing[2]);//.normalized();

	data->GetPointData()->GetScalars()->GetRange(mRange);
	LOGD("range min=%f max=%f", mRange[0], mRange[1]);

	vtkDataArray* scalars = data->GetPointData()->GetScalars();
	android_assert(scalars);

	unsigned int num = scalars->GetNumberOfTuples();
	android_assert(num == static_cast<unsigned>(mDimensions[0]*mDimensions[1]*mDimensions[2]));
	mTexture.resize(num*4);

	for (unsigned int i = 0; i < num; ++i) {
		double value = scalars->GetComponent(i, 0);
		double norm = (value-mRange[0]) / (mRange[1]-mRange[0]);
		float r, g, b;
		colormap(norm, r, g, b);
		mTexture[i*4+0] = r*255;
		mTexture[i*4+1] = g*255;
		mTexture[i*4+2] = b*255;
		// mTexture[i*4+3] = (0.03+0.3*norm)*255;
		mTexture[i*4+3] = (0.03+0.97*norm)*255; // alpha
		// mTexture[i*4+3] = 0.3*norm*255;
		// mTexture[i*4+3] = (0.3+0.7*norm)*255;
		// mTexture[i*4+3] = (0.1+0.7*norm)*255;
		// mTexture[i*4+3] = norm*255;
	}

	// vtkNew<vtkExtractVOI> sliceFilter;
	// sliceFilter->SetInputData(data);

	// // X slices
	// for (int i = 0; i < mDimensions[0]; i += step) {
	// 	sliceFilter->SetVOI(
	// 		i, i,
	// 		0, mDimensions[1],
	// 		0, mDimensions[2]
	// 	);
	// 	sliceFilter->Update();

	// 	vtkImageData* slice = sliceFilter->GetOutput();
	// 	android_assert(slice);

	// 	vtkDataArray* scalars = slice->GetPointData()->GetScalars();
	// 	android_assert(scalars);

	// 	unsigned int num = scalars->GetNumberOfTuples();
	// 	android_assert(num == static_cast<unsigned>(mDimensions[1]*mDimensions[2]));
	// 	mTexturesX.emplace_back(num*4);

	// 	for (unsigned int j = 0; j < num; ++j) {
	// 		double value = scalars->GetComponent(j, 0);
	// 		double norm = (value-range[0]) / (range[1]-range[0]);
	// 		float r, g, b;
	// 		colormap(norm, r, g, b);
	// 		mTexturesX.back()[j*4+0] = r*255;
	// 		mTexturesX.back()[j*4+1] = g*255;
	// 		mTexturesX.back()[j*4+2] = b*255;
	// 		mTexturesX.back()[j*4+3] = (0.03+0.3*norm)*255;
	// 	}
	// }

	// // Y slices
	// for (int i = 0; i < mDimensions[1]; i += step) {
	// 	sliceFilter->SetVOI(
	// 		0, mDimensions[0],
	// 		i, i,
	// 		0, mDimensions[2]
	// 	);
	// 	sliceFilter->Update();

	// 	vtkImageData* slice = sliceFilter->GetOutput();
	// 	android_assert(slice);

	// 	vtkDataArray* scalars = slice->GetPointData()->GetScalars();
	// 	android_assert(scalars);

	// 	unsigned int num = scalars->GetNumberOfTuples();
	// 	android_assert(num == static_cast<unsigned>(mDimensions[0]*mDimensions[2]));
	// 	mTexturesY.emplace_back(num*4);

	// 	for (unsigned int j = 0; j < num; ++j) {
	// 		double value = scalars->GetComponent(j, 0);
	// 		double norm = (value-range[0]) / (range[1]-range[0]);
	// 		float r, g, b;
	// 		colormap(norm, r, g, b);
	// 		mTexturesY.back()[j*4+0] = r*255;
	// 		mTexturesY.back()[j*4+1] = g*255;
	// 		mTexturesY.back()[j*4+2] = b*255;
	// 		mTexturesY.back()[j*4+3] = (0.03+0.3*norm)*255;
	// 	}
	// }

	// // Z slices
	// for (int i = 0; i < mDimensions[2]; i += step) {
	// 	sliceFilter->SetVOI(
	// 		0, mDimensions[0],
	// 		0, mDimensions[1],
	// 		i, i
	// 	);
	// 	sliceFilter->Update();

	// 	vtkImageData* slice = sliceFilter->GetOutput();
	// 	android_assert(slice);

	// 	vtkDataArray* scalars = slice->GetPointData()->GetScalars();
	// 	android_assert(scalars);

	// 	unsigned int num = scalars->GetNumberOfTuples();
	// 	android_assert(num == static_cast<unsigned>(mDimensions[0]*mDimensions[1]));
	// 	mTexturesZ.emplace_back(num*4);

	// 	for (unsigned int j = 0; j < num; ++j) {
	// 		double value = scalars->GetComponent(j, 0);
	// 		double norm = (value-range[0]) / (range[1]-range[0]);
	// 		float r, g, b;
	// 		colormap(norm, r, g, b);
	// 		mTexturesZ.back()[j*4+0] = r*255;
	// 		mTexturesZ.back()[j*4+1] = g*255;
	// 		mTexturesZ.back()[j*4+2] = b*255;
	// 		mTexturesZ.back()[j*4+3] = (0.03+0.3*norm)*255;
	// 	}
	// }

	int extent[6];
	data->GetExtent(extent);
	LOGD("%i %i %i %i %i %i extent", extent[0], extent[1], extent[2], extent[3], extent[4], extent[5]);
	LOGD("loading finished");
}

Volume::~Volume()
{
	// Schedule texture to be cleared
	if (mTextureHandle != 0) {
		synchronized (staleTexturesList) {
			staleTexturesList.emplace_back(mTextureHandle);
		}
	}
}


bool Volume::hasClipPlane()
{
	// return !__isinf(mClipEq[3]);
	return (mClipEq[3] < std::numeric_limits<float>::max());
}

void Volume::setClipPlane(float a, float b, float c, float d)
{
	mClipEq[0] = a;
	mClipEq[1] = b;
	mClipEq[2] = c;
	mClipEq[3] = d;
}

void Volume::clearClipPlane()
{
	mClipEq[0] = mClipEq[1] = mClipEq[2] = 0;
	// mClipEq[3] = -std::numeric_limits<float>::infinity();
	mClipEq[3] = std::numeric_limits<float>::max();
}

// (GL context)
void Volume::switchMaterial(MaterialSharedPtr newMaterial)
{
	if (mMaterial == newMaterial)
		return;

	android_assert(newMaterial);
	mMaterial = newMaterial;
	// LOGD("switching to volume material: %s", (newMaterial == mMaterialFast ? "fast" : "clip"));

	mMaterial->bind();

	mVertexAttrib = mMaterial->getAttribute("vertex");
	mTexCoordAttrib = mMaterial->getAttribute("texCoord");
	mModelViewUniform = mMaterial->getUniform("modelView");
	mProjectionUniform = mMaterial->getUniform("projection");
	mDimensionsUniform = mMaterial->getUniform("dimensions");
	mInvertUniform = mMaterial->getUniform("invert");
	mClipPlaneUniform = mMaterial->getUniform("clipPlane");
	mSpacingUniform = mMaterial->getUniform("spacing");
	mOpacityUniform = mMaterial->getUniform("opacity");

	android_assert(mVertexAttrib != -1);
	android_assert(mTexCoordAttrib != -1);
	android_assert(mModelViewUniform != -1);
	android_assert(mProjectionUniform != -1);
	android_assert(mDimensionsUniform != -1);
	android_assert(mInvertUniform != -1);
	if (hasClipPlane()) android_assert(mClipPlaneUniform != -1);
	android_assert(mSpacingUniform != -1);
	android_assert(mOpacityUniform != -1);
}

// (GL context)
void Volume::bind()
{
	if (mMaterial)
		mMaterial->bind();

	// Required because input is not RGBA (i.e. not aligned to a 4-byte boundary)
	// http://www.opengl.org/wiki/Common_Mistakes#Texture_upload_and_pixel_reads
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &mTextureHandle);
	glBindTexture(GL_TEXTURE_3D/*_OES*/, mTextureHandle);
	glTexParameteri(GL_TEXTURE_3D/*_OES*/, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D/*_OES*/, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D/*_OES*/, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D/*_OES*/, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D/*_OES*/, GL_TEXTURE_WRAP_R/*_OES*/, GL_CLAMP_TO_EDGE);

	// PFNGLTEXIMAGE3DOESPROC glTexImage3DOES = nullptr;
	// glTexImage3DOES = GetProcAddress<PFNGLTEXIMAGE3DOESPROC>("glTexImage3DOES");

	// Initialize the texture
	// glTexImage3DOES(
	glTexImage3D(
		GL_TEXTURE_3D/*_OES*/,
		0,
		GL_RGBA,
		mDimensions[0], mDimensions[1], mDimensions[2],
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		mTexture.data()
	);

	unsigned int baseIndex = 0;
	initXPlanes(baseIndex);
	initYPlanes(baseIndex);
	initZPlanes(baseIndex);

	// Allocate 5 VBOs
	GLuint vbos[5];
	glGenBuffers(5, vbos);

	// Vertices
	mVertexBuffer = vbos[0];
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, mVertices.size()*sizeof(GLfloat),
	             mVertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Texture coordinates
	mTexCoordBuffer = vbos[1];
	glBindBuffer(GL_ARRAY_BUFFER, mTexCoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, mTexCoords.size()*sizeof(GLfloat),
	             mTexCoords.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Indices X
	mIndexBufferX = vbos[2];
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferX);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesX.size()*sizeof(GLushort),
	             mIndicesX.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Indices Y
	mIndexBufferY = vbos[3];
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferY);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesY.size()*sizeof(GLushort),
	             mIndicesY.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Indices Z
	mIndexBufferZ = vbos[4];
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferZ);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesZ.size()*sizeof(GLushort),
	             mIndicesZ.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	mBound = true;
}

void Volume::initXPlanes(unsigned int& baseIndex)
{
	bool createBuffer = mIndicesX.empty();

	// glGenTextures(1, &mTextureXHandle);
	// glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mTextureXHandle);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// // Initialize the texture with no content
	// glTexImage3DOES(
	// 	GL_TEXTURE_2D_ARRAY_EXT,
	// 	0,
	// 	GL_RGBA,
	// 	mDimensions[1], mDimensions[2], mTexturesX.size(),
	// 	0,
	// 	GL_RGBA,
	// 	GL_UNSIGNED_BYTE,
	// 	nullptr
	// );

	// unsigned int i = 0;
	// for (const auto& slice : mTexturesX) {
	// for (unsigned int i = 0; i < mDimensions[0]/step; ++i) {
	for (int i = 0; i < mDimensions[0]; i+=step) {
		// glTexSubImage3DOES(
		// 	GL_TEXTURE_2D_ARRAY_EXT,
		// 	0,
		// 	0, 0, i,
		// 	mDimensions[1], mDimensions[2], 1,
		// 	GL_RGBA,
		// 	GL_UNSIGNED_BYTE,
		// 	slice.data()
		// );

		if (createBuffer) {
			mVertices.push_back(float(i)/mDimensions[0] - 0.5f);
			mVertices.push_back(-0.5f);
			mVertices.push_back(-0.5f);
			mTexCoords.push_back(i/float(mDimensions[0]));
			mTexCoords.push_back(0);
			mTexCoords.push_back(1);

			mVertices.push_back(float(i)/mDimensions[0] - 0.5f);
			mVertices.push_back(-0.5f);
			mVertices.push_back(0.5f);
			mTexCoords.push_back(i/float(mDimensions[0]));
			mTexCoords.push_back(0);
			mTexCoords.push_back(0);

			mVertices.push_back(float(i)/mDimensions[0] - 0.5f);
			mVertices.push_back(0.5f);
			mVertices.push_back(0.5f);
			mTexCoords.push_back(i/float(mDimensions[0]));
			mTexCoords.push_back(1);
			mTexCoords.push_back(0);

			mVertices.push_back(float(i)/mDimensions[0] - 0.5f);
			mVertices.push_back(0.5f);
			mVertices.push_back(-0.5f);
			mTexCoords.push_back(i/float(mDimensions[0]));
			mTexCoords.push_back(1);
			mTexCoords.push_back(1);

			mIndicesX.push_back(baseIndex*4+0);
			mIndicesX.push_back(baseIndex*4+2);
			mIndicesX.push_back(baseIndex*4+1);

			mIndicesX.push_back(baseIndex*4+0);
			mIndicesX.push_back(baseIndex*4+3);
			mIndicesX.push_back(baseIndex*4+2);
		}

		// ++i;
		++baseIndex;
	}
}

void Volume::initYPlanes(unsigned int& baseIndex)
{
	bool createBuffer = mIndicesY.empty();

	// glGenTextures(1, &mTextureYHandle);
	// glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mTextureYHandle);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// // Initialize the texture with no content
	// glTexImage3DOES(
	// 	GL_TEXTURE_2D_ARRAY_EXT,
	// 	0,
	// 	GL_RGBA,
	// 	mDimensions[0], mDimensions[2], mTexturesY.size(),
	// 	0,
	// 	GL_RGBA,
	// 	GL_UNSIGNED_BYTE,
	// 	nullptr
	// );

	// unsigned int i = 0;
	// for (const auto& slice : mTexturesY) {
	for (int i = 0; i < mDimensions[1]; i+=step) {
		// glTexSubImage3DOES(
		// 	GL_TEXTURE_2D_ARRAY_EXT,
		// 	0,
		// 	0, 0, i,
		// 	mDimensions[0], mDimensions[2], 1,
		// 	GL_RGBA,
		// 	GL_UNSIGNED_BYTE,
		// 	slice.data()
		// );

		if (createBuffer) {
			mVertices.push_back(-0.5f);
			mVertices.push_back(float(i)/mDimensions[1] - 0.5f);
			mVertices.push_back(-0.5f);
			mTexCoords.push_back(0);
			mTexCoords.push_back(i/float(mDimensions[1]));
			mTexCoords.push_back(1);

			mVertices.push_back(-0.5f);
			mVertices.push_back(float(i)/mDimensions[1] - 0.5f);
			mVertices.push_back(0.5f);
			mTexCoords.push_back(0);
			mTexCoords.push_back(i/float(mDimensions[1]));
			mTexCoords.push_back(0);

			mVertices.push_back(0.5f);
			mVertices.push_back(float(i)/mDimensions[1] - 0.5f);
			mVertices.push_back(0.5f);
			mTexCoords.push_back(1);
			mTexCoords.push_back(i/float(mDimensions[1]));
			mTexCoords.push_back(0);

			mVertices.push_back(0.5f);
			mVertices.push_back(float(i)/mDimensions[1] - 0.5f);
			mVertices.push_back(-0.5f);
			mTexCoords.push_back(1);
			mTexCoords.push_back(i/float(mDimensions[1]));
			mTexCoords.push_back(1);

			mIndicesY.push_back(baseIndex*4+0);
			mIndicesY.push_back(baseIndex*4+2);
			mIndicesY.push_back(baseIndex*4+1);

			mIndicesY.push_back(baseIndex*4+0);
			mIndicesY.push_back(baseIndex*4+3);
			mIndicesY.push_back(baseIndex*4+2);
		}

		// ++i;
		++baseIndex;
	}
}

void Volume::initZPlanes(unsigned int& baseIndex)
{
	bool createBuffer = mIndicesZ.empty();

	// glGenTextures(1, &mTextureZHandle);
	// glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mTextureZHandle);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// // Initialize the texture with no content
	// glTexImage3DOES(
	// 	GL_TEXTURE_2D_ARRAY_EXT,
	// 	0,
	// 	GL_RGBA,
	// 	mDimensions[0], mDimensions[1], mTexturesZ.size(),
	// 	0,
	// 	GL_RGBA,
	// 	GL_UNSIGNED_BYTE,
	// 	nullptr
	// );

	// unsigned int i = ;
	// for (const auto& slice : mTexturesZ) {
	for (int i = mDimensions[2]; i >= 0; i-=step) {
	// for (int i = 0; i < mDimensions[2]; ++i) {
		// glTexSubImage3DOES(
		// 	GL_TEXTURE_2D_ARRAY_EXT,
		// 	0,
		// 	0, 0, i,
		// 	mDimensions[0], mDimensions[1], 1,
		// 	GL_RGBA,
		// 	GL_UNSIGNED_BYTE,
		// 	slice.data()
		// );

		if (createBuffer) {
			mVertices.push_back(-0.5f);
			mVertices.push_back(-0.5f);
			mVertices.push_back(float(i)/mDimensions[2] - 0.5f);
			mTexCoords.push_back(0);
			mTexCoords.push_back(0);
			mTexCoords.push_back(1-i/float(mDimensions[2]));

			mVertices.push_back(-0.5f);
			mVertices.push_back(0.5f);
			mVertices.push_back(float(i)/mDimensions[2] - 0.5f);
			mTexCoords.push_back(0);
			mTexCoords.push_back(1);
			mTexCoords.push_back(1-i/float(mDimensions[2]));

			mVertices.push_back(0.5f);
			mVertices.push_back(0.5f);
			mVertices.push_back(float(i)/mDimensions[2] - 0.5f);
			mTexCoords.push_back(1);
			mTexCoords.push_back(1);
			mTexCoords.push_back(1-i/float(mDimensions[2]));

			mVertices.push_back(0.5f);
			mVertices.push_back(-0.5f);
			mVertices.push_back(float(i)/mDimensions[2] - 0.5f);
			mTexCoords.push_back(1);
			mTexCoords.push_back(0);
			mTexCoords.push_back(1-i/float(mDimensions[2]));

			mIndicesZ.push_back(baseIndex*4+0);
			mIndicesZ.push_back(baseIndex*4+2);
			mIndicesZ.push_back(baseIndex*4+1);

			mIndicesZ.push_back(baseIndex*4+0);
			mIndicesZ.push_back(baseIndex*4+3);
			mIndicesZ.push_back(baseIndex*4+2);
		}

		// --i;
		++baseIndex;
	}
}

// (GL context)
void Volume::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	synchronized (staleTexturesList) {
		if (!staleTexturesList.empty()) {
			LOGD("freeing %ld texture(s)", staleTexturesList.size());
			for (GLuint texture : staleTexturesList)
				glDeleteTextures(1, &texture);
			staleTexturesList.clear();
		}
	}

	if (!mBound)
		bind();

	switchMaterial(hasClipPlane() ? mMaterialClip : mMaterialFast);
	android_assert(mMaterial);

	// Vertices
	android_assert(mVertexBuffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	android_assert(mVertexAttrib != -1);
	glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(mVertexAttrib);

	// Texture coordinates
	android_assert(mTexCoordBuffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, mTexCoordBuffer);
	android_assert(mTexCoordAttrib != -1);
	glVertexAttribPointer(mTexCoordAttrib, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(mTexCoordAttrib);

	// Uniforms
	glUseProgram(mMaterial->getHandle());
	glUniformMatrix4fv(mProjectionUniform, 1, false, projectionMatrix.data_);
	glUniform4fv(mClipPlaneUniform, 1, mClipEq);
	glUniform3f(mSpacingUniform, mSpacing.x, mSpacing.y, mSpacing.z);
	glUniform1f(mOpacityUniform, mOpacity);
	//For the selection
	glUniform1i(mMaterial->getUniform("selectionMode"), false);

	int dimVec[3];
	dimVec[0] = mDimensions[0];
	dimVec[1] = mDimensions[1];
	dimVec[2] = mDimensions[2];

	// glActiveTexture(GL_TEXTURE0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D/*_OES*/, mTextureHandle);

	Matrix3 normalMatrix = modelViewMatrix.inverse().transpose().get3x3Matrix();

	float xDot = (normalMatrix*Vector3::unitX()).normalized().dot(Vector3::unitZ());
	float yDot = (normalMatrix*Vector3::unitY()).normalized().dot(Vector3::unitZ());
	// // Reduce the weight of the vertical axis, since it looks
	// // worse than other axis when viewed from an average angle
	// float zDot = (normalMatrix*Vector3::unitZ()).normalized().dot(Vector3::unitZ())*0.995;
	float zDot = (normalMatrix*Vector3::unitZ()).normalized().dot(Vector3::unitZ());

	// LOGD("xDot = %f, yDot = %f, zDot = %f", xDot, yDot, zDot);
	static const Matrix4 xInv = Matrix4::makeTransform(Vector3::zero(), Quaternion::identity(), Vector3(-1.0, 1.0, 1.0));
	static const Matrix4 yInv = Matrix4::makeTransform(Vector3::zero(), Quaternion::identity(), Vector3(1.0, -1.0, 1.0));
	static const Matrix4 zInv = Matrix4::makeTransform(Vector3::zero(), Quaternion::identity(), Vector3(1.0, 1.0, -1.0));

	// Display the planes whose normal is closest to the screen normal
	// http://prosjekt.ffi.no/unik-4660/lectures04/chapters/Voxel1.html
	if (std::abs(xDot) > std::abs(yDot) && std::abs(xDot) > std::abs(zDot)) {
		// LOGD("largest: xDot (%f)", xDot);
		// dimVec[3] = mTexturesX.size();
		// dimVec[3] = mDimensions[0];
		glUniform3iv(mDimensionsUniform, 1, dimVec);
		if (xDot < 0) {
			glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
			// glUniform1i(mInvertUniform, 0);
			glUniform3f(mInvertUniform, 0, 0, 0);
		} else {
			glUniformMatrix4fv(mModelViewUniform, 1, false, (modelViewMatrix * xInv).data_);
			// glUniform1i(mInvertUniform, 1);
			glUniform3f(mInvertUniform, 1, 0, 0);
		}
		// X planes
		android_assert(mIndexBufferX != 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferX);
		// android_assert(mTextureXHandle != 0);
		// glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mTextureXHandle);
		glDrawElements(GL_TRIANGLES, mIndicesX.size(), GL_UNSIGNED_SHORT, nullptr);

	} else if (std::abs(yDot) > std::abs(xDot) && std::abs(yDot) > std::abs(zDot)) {
		// LOGD("largest: yDot (%f)", yDot);
		// dimVec[3] = mTexturesY.size();
		// dimVec[3] = mDimensions[1];
		glUniform3iv(mDimensionsUniform, 1, dimVec);
		if (yDot < 0) {
			glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
			// glUniform1i(mInvertUniform, 0);
			glUniform3f(mInvertUniform, 0, 0, 0);
		} else {
			glUniformMatrix4fv(mModelViewUniform, 1, false, (modelViewMatrix * yInv).data_);
			// glUniform1i(mInvertUniform, 1);
			glUniform3f(mInvertUniform, 0, 1, 0);
		}
		// Y planes
		android_assert(mIndexBufferY != 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferY);
		// android_assert(mTextureYHandle != 0);
		// glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mTextureYHandle);
		glDrawElements(GL_TRIANGLES, mIndicesY.size(), GL_UNSIGNED_SHORT, nullptr);

	} else {
		// LOGD("largest: zDot (%f)", zDot);
		// dimVec[3] = mTexturesZ.size();
		// dimVec[3] = mDimensions[2];
		glUniform3iv(mDimensionsUniform, 1, dimVec);
		if (zDot < 0) {
			glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
			// glUniform1i(mInvertUniform, 0);
			glUniform3f(mInvertUniform, 0, 0, 0);
		} else {
			glUniformMatrix4fv(mModelViewUniform, 1, false, (modelViewMatrix * zInv).data_);
			// glUniform1i(mInvertUniform, 1);
			glUniform3f(mInvertUniform, 0, 0, 1);
		}
		// Z planes
		android_assert(mIndexBufferZ != 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferZ);
		// android_assert(mTextureZHandle != 0);
		// glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mTextureZHandle);
		glDrawElements(GL_TRIANGLES, mIndicesZ.size(), GL_UNSIGNED_SHORT, nullptr);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(mVertexAttrib);
	glDisableVertexAttribArray(mTexCoordAttrib);
}

// (GL context)
void Volume::renderSelection(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix, Vector3& firstPos, Vector3& lastPos, const Matrix4& invSelectionMatrix, const Matrix4& oldData)
{
	synchronized (staleTexturesList) {
		if (!staleTexturesList.empty()) {
			LOGD("freeing %ld texture(s)", staleTexturesList.size());
			for (GLuint texture : staleTexturesList)
				glDeleteTextures(1, &texture);
			staleTexturesList.clear();
		}
	}

	if (!mBound)
		bind();

	switchMaterial(hasClipPlane() ? mMaterialClip : mMaterialFast);
	android_assert(mMaterial);

	// Vertices
	android_assert(mVertexBuffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	android_assert(mVertexAttrib != -1);
	glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(mVertexAttrib);

	// Texture coordinates
	android_assert(mTexCoordBuffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, mTexCoordBuffer);
	android_assert(mTexCoordAttrib != -1);
	glVertexAttribPointer(mTexCoordAttrib, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(mTexCoordAttrib);

	// Uniforms
	glUseProgram(mMaterial->getHandle());
	glUniformMatrix4fv(mProjectionUniform, 1, false, projectionMatrix.data_);
	glUniform4fv(mClipPlaneUniform, 1, mClipEq);
	glUniform3f(mSpacingUniform, mSpacing.x, mSpacing.y, mSpacing.z);
	glUniform1f(mOpacityUniform, mOpacity);
	//For the selection
	glUniform1i(mMaterial->getUniform("selectionMode"), true);
	glUniform3f(mMaterial->getUniform("uFirstPos"), firstPos.x, firstPos.y, firstPos.z);
	glUniform3f(mMaterial->getUniform("uLastPos"), lastPos.x, lastPos.y, lastPos.z);
	glUniformMatrix4fv(mMaterial->getUniform("selectionMat"), 1, false, invSelectionMatrix.data_);
	glUniformMatrix4fv(mMaterial->getUniform("postTreatment"), 1, false, oldData.data_);

	
	int dimVec[3];
	dimVec[0] = mDimensions[0];
	dimVec[1] = mDimensions[1];
	dimVec[2] = mDimensions[2];

	// glActiveTexture(GL_TEXTURE0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D/*_OES*/, mTextureHandle);

	Matrix3 normalMatrix = modelViewMatrix.inverse().transpose().get3x3Matrix();

	float xDot = (normalMatrix*Vector3::unitX()).normalized().dot(Vector3::unitZ());
	float yDot = (normalMatrix*Vector3::unitY()).normalized().dot(Vector3::unitZ());
	// // Reduce the weight of the vertical axis, since it looks
	// // worse than other axis when viewed from an average angle
	// float zDot = (normalMatrix*Vector3::unitZ()).normalized().dot(Vector3::unitZ())*0.995;
	float zDot = (normalMatrix*Vector3::unitZ()).normalized().dot(Vector3::unitZ());

	// LOGD("xDot = %f, yDot = %f, zDot = %f", xDot, yDot, zDot);
	static const Matrix4 xInv = Matrix4::makeTransform(Vector3::zero(), Quaternion::identity(), Vector3(-1.0, 1.0, 1.0));
	static const Matrix4 yInv = Matrix4::makeTransform(Vector3::zero(), Quaternion::identity(), Vector3(1.0, -1.0, 1.0));
	static const Matrix4 zInv = Matrix4::makeTransform(Vector3::zero(), Quaternion::identity(), Vector3(1.0, 1.0, -1.0));

	// Display the planes whose normal is closest to the screen normal
	// http://prosjekt.ffi.no/unik-4660/lectures04/chapters/Voxel1.html
	if (std::abs(xDot) > std::abs(yDot) && std::abs(xDot) > std::abs(zDot)) {
		// LOGD("largest: xDot (%f)", xDot);
		// dimVec[3] = mTexturesX.size();
		// dimVec[3] = mDimensions[0];
		glUniform3iv(mDimensionsUniform, 1, dimVec);
		if (xDot < 0) {
			glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
			// glUniform1i(mInvertUniform, 0);
			glUniform3f(mInvertUniform, 0, 0, 0);
		} else {
			glUniformMatrix4fv(mModelViewUniform, 1, false, (modelViewMatrix * xInv).data_);
			// glUniform1i(mInvertUniform, 1);
			glUniform3f(mInvertUniform, 1, 0, 0);
		}
		// X planes
		android_assert(mIndexBufferX != 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferX);
		// android_assert(mTextureXHandle != 0);
		// glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mTextureXHandle);
		glDrawElements(GL_TRIANGLES, mIndicesX.size(), GL_UNSIGNED_SHORT, nullptr);

	} else if (std::abs(yDot) > std::abs(xDot) && std::abs(yDot) > std::abs(zDot)) {
		// LOGD("largest: yDot (%f)", yDot);
		// dimVec[3] = mTexturesY.size();
		// dimVec[3] = mDimensions[1];
		glUniform3iv(mDimensionsUniform, 1, dimVec);
		if (yDot < 0) {
			glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
			// glUniform1i(mInvertUniform, 0);
			glUniform3f(mInvertUniform, 0, 0, 0);
		} else {
			glUniformMatrix4fv(mModelViewUniform, 1, false, (modelViewMatrix * yInv).data_);
			// glUniform1i(mInvertUniform, 1);
			glUniform3f(mInvertUniform, 0, 1, 0);
		}
		// Y planes
		android_assert(mIndexBufferY != 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferY);
		// android_assert(mTextureYHandle != 0);
		// glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mTextureYHandle);
		glDrawElements(GL_TRIANGLES, mIndicesY.size(), GL_UNSIGNED_SHORT, nullptr);

	} else {
		// LOGD("largest: zDot (%f)", zDot);
		// dimVec[3] = mTexturesZ.size();
		// dimVec[3] = mDimensions[2];
		glUniform3iv(mDimensionsUniform, 1, dimVec);
		if (zDot < 0) {
			glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
			// glUniform1i(mInvertUniform, 0);
			glUniform3f(mInvertUniform, 0, 0, 0);
		} else {
			glUniformMatrix4fv(mModelViewUniform, 1, false, (modelViewMatrix * zInv).data_);
			// glUniform1i(mInvertUniform, 1);
			glUniform3f(mInvertUniform, 0, 0, 1);
		}
		// Z planes
		android_assert(mIndexBufferZ != 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferZ);
		// android_assert(mTextureZHandle != 0);
		// glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mTextureZHandle);
		glDrawElements(GL_TRIANGLES, mIndicesZ.size(), GL_UNSIGNED_SHORT, nullptr);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(mVertexAttrib);
	glDisableVertexAttribArray(mTexCoordAttrib);
}
