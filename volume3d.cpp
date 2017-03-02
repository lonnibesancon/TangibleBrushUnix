#include "volume3d.h"

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
	const char* backFaceVertexShader =
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
		"attribute highp vec3 vertex;\n"
		"varying highp vec3 v_color;\n" // output color = vertex position (interpolated)

		"void main() {\n"
		"  mediump vec3 scale = vec3(float(dimensions.x), float(dimensions.y), float(dimensions.z)) * spacing;\n"
		"  highp vec4 viewSpacePos = modelView * vec4(scale * (vertex * vec3(1.0, 1.0, -1.0)), 1.0);\n"
		"  gl_Position = projection * viewSpacePos;\n"
		"  v_color = vertex;\n"
		"}";

	const char* backFaceFragmentShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"varying highp vec3 v_color;\n"

		"void main() {\n"
		"  gl_FragColor = vec4(v_color, 1.0);\n"
		"}";

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
		"attribute highp vec3 vertex;\n"

		// FIXME
		"uniform highp vec4 clipPlane;\n"
		"varying highp float v_clipDist;\n"

		"varying highp vec3 v_entryPoint;\n"
		// "varying highp vec4 v_exitPoint;\n"
		"varying highp vec4 v_pos;\n"

		"void main() {\n"
		"  mediump vec3 scale = vec3(float(dimensions.x), float(dimensions.y), float(dimensions.z)) * spacing;\n"
		"  highp vec4 viewSpacePos = modelView * vec4(scale * (vertex * vec3(1.0, 1.0, -1.0)), 1.0);\n"
		"  gl_Position = projection * viewSpacePos;\n"

		"  v_clipDist = dot(viewSpacePos.xyz, clipPlane.xyz) + clipPlane.w;\n"

		"  v_entryPoint = vertex;\n"
		"  v_pos = gl_Position;\n"
		// "  v_exitPoint = gl_Position;\n"

		"}";

	// #ifndef GL_TEXTURE_3D_OES
	// 	#define GL_TEXTURE_3D_OES 0x806F
	// #endif
	//
	// #ifndef GL_TEXTURE_WRAP_R_OES
	// 	#define GL_TEXTURE_WRAP_R_OES 0x8072
	// #endif

	// FIXME: hardcoded values
	// See also: glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_{WIDTH/HEIGHT}, ...)
	// static const unsigned int screenWidth = 1920;
	// static const unsigned int screenHeight = 1104;
	static const unsigned int screenWidth = 800;
	static const unsigned int screenHeight = 460;

	const char* fragmentShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"#extension GL_OES_texture_3D : require\n"
		"uniform lowp sampler3D texture;\n"
		"uniform lowp sampler2D backFaceTexture;\n"
		// "uniform lowp vec2 screenSize;\n"

		"varying highp float v_clipDist;\n" // FIXME

		"varying highp vec3 v_entryPoint;\n"
		// "varying highp vec4 v_exitPoint;\n"
		"varying highp vec4 v_pos;\n"
		"varying highp vec4 v_posNotModified;\n"

		"void main() {\n"

		"  if (v_clipDist <= 0.0) discard;\n" // FIXME

		// "  highp vec3 exitPoint = texture2D(backFaceTexture, gl_FragCoord.st/screenSize).xyz;\n"
		"  highp vec3 exitPoint = texture2D(backFaceTexture, ((v_pos.xy/v_pos.w)+1.0)/2.0).xyz;\n"
		// "  if (v_entryPoint == exitPoint) discard;\n" // background need no raycasting

		// "  highp vec3 dir = exitPoint - v_entryPoint;\n"
		// "  highp float len = length(dir);\n"
		// // "  lowp float stepSize = 0.001;\n"
		// "  lowp float stepSize = 0.1;\n"
		// "  highp vec3 deltaDir = normalize(dir) * stepSize;\n"
		// "  highp float deltaDirLen = length(deltaDir);\n"
		// "  highp vec3 voxelCoord = v_entryPoint;\n"
		// "  lowp vec4 colorAccum = vec4(0.0);\n"
		// "  lowp float alphaAccum = 0.0;\n"
		// "  highp float lengthAccum = 0.0;\n"
		// "  lowp vec4 bgColor = vec4(1.0, 1.0, 1.0, 0.0);\n"

		// // "  for (int i = 0; i < 450; i++) {\n"
		// "  for (int i = 0; i < 20; i++) {\n"
		// "    lowp vec4 colorSample = texture3D(texture, voxelCoord);\n"
		// "    lowp float alphaSample = colorSample.a * stepSize;\n"
		// "    colorAccum += (1.0 - alphaAccum) * colorSample * alphaSample*3.0;\n"
		// "    alphaAccum += alphaSample;\n"
		// "    voxelCoord += deltaDir;\n"
		// "    lengthAccum += deltaDirLen;\n"
		// "    if (lengthAccum >= len || alphaAccum > 1.0) break;\n" // terminate if opacity > 1 or the ray is outside the volume
	    // "  }\n"

		// "  highp vec3 dir = exitPoint - v_entryPoint;\n"
		"  highp vec3 dir = v_entryPoint - exitPoint;\n" // FIXME: entry/exit points are inverted for some reason
		// "  highp vec3 deltaDir = dir / 15.0;\n" // 15 = num steps
		"  highp vec3 deltaDir = normalize(dir) * 0.01;\n" // 0.01: step size
		// "  highp vec3 deltaDir = normalize(dir) * 0.05;\n" // 0.05: step size
		"  highp float len = length(dir);\n"
		"  highp float deltaLen = length(deltaDir);\n"
		// "  highp vec3 pos = v_entryPoint;\n"
		"  highp vec3 pos = exitPoint;\n"
		"  lowp vec4 colorAccum = vec4(0.0);\n"
		"  highp float lengthAcc = 0.0;\n"
		// "  for (int i = 0; i < 15; i++) {\n"
		"  for (int i = 0; i < 100; i++) {\n"
		// "    lowp vec4 colorSample = texture3D(texture, pos);\n"
		"    lowp vec4 colorSample = texture3D(texture, vec3(pos.x, pos.y, 1.0-pos.z));\n"
		// "    colorAccum += (1.0 - colorAccum.a) * colorAccum + colorSample * colorSample.a;\n"
		// "    colorAccum += vec4(colorSample.xyz * colorSample.a, colorSample.a);\n"
		// "    colorAccum.a = 1.0 - pow(1.0 - colorSample.a, 0.01*200.0);\n"
		// "    colorAccum.rgb += (1.0 - colorAccum.a) * colorSample.rgb * colorSample.a;\n"
		// "    colorAccum.a += (1.0 - colorAccum.a) * colorSample.a;\n"
		// "    colorAccum.xyz += colorSample.xyz/15.0;\n" // 15 = num steps
		// "    colorAccum.xyz += colorSample.xyz * 0.1;\n"
		// "   lowp float alpha = pow(colorSample.a, 2.0);\n"
		// "    lowp float alpha = pow(colorSample.a, 0.9);\n" // for step size = 0.05
		"    lowp float alpha = pow(colorSample.a, 1.3);\n" // for step size = 0.01
		// "    lowp float alpha = colorSample.a;\n"
		"    colorAccum.a += alpha;\n"
		"    colorAccum.xyz += colorSample.xyz * alpha;\n"
		// "    colorAccum.xyz += mix(colorAccum.xyz, colorSample.xyz, alpha);\n"
		// "    colorAccum.a += colorSample.a;\n"
		"    pos += deltaDir;\n"
		"    lengthAcc += deltaLen;\n"
		"    if (lengthAcc >= len || colorAccum.a >= 1.0) break;\n"
		// "    if (lengthAcc >= len) break;\n"
	    "  }\n"

		"  gl_FragColor = colorAccum;\n"
		// "  gl_FragColor = vec4(vec3(length(dir)), 1.0);\n"
		// "  gl_FragColor = texture3D(texture, exitPoint);\n"

		"}";

	void colormap(double value, float& r, float& g, float& b_)
	{
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

	void checkError(const char* msg)
	{
		GLenum err;
		while ((err = glGetError()) != 0) {
			LOGD("%s: error = %d", msg, err);
		}
	}
	#define CHECK(x) (x); checkError(#x);
	// #define CHECK(x) (x)

	Synchronized<std::list<GLuint>> staleTexturesList;
} // namespace

Volume3d::Volume3d(vtkSmartPointer<vtkImageData> data)
 : mMaterial(MaterialSharedPtr(new Material(vertexShader, fragmentShader))),
   mBackFaceMaterial(MaterialSharedPtr(new Material(backFaceVertexShader, backFaceFragmentShader))),
   mBound(false),
   mVertexAttrib(-1), mBFVertexAttrib(-1),
   mProjectionUniform(-1), mModelViewUniform(-1), mDimensionsUniform(-1), mClipPlaneUniform(-1), mSpacingUniform(-1), mScreenSizeUniform(-1),
   mBFProjectionUniform(-1), mBFModelViewUniform(-1), mBFDimensionsUniform(-1), mBFSpacingUniform(-1),
   mTextureHandle(0), mBackFaceTextureHandle(0),
   mFrameBuffer(0)
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
		mTexture[i*4+3] = (0.03+0.3*norm)*255;
		// mTexture[i*4+3] = (0.03+0.97*norm)*255;
		// mTexture[i*4+3] = 0.3*norm*255;
		// mTexture[i*4+3] = (0.3+0.7*norm)*255;
		// mTexture[i*4+3] = (0.1+0.7*norm)*255;
		// mTexture[i*4+3] = norm*255;
	}

	LOGD("loading finished");
}

Volume3d::~Volume3d()
{
	// Schedule texture to be cleared
	synchronized (staleTexturesList) {
		if (mTextureHandle != 0)
			staleTexturesList.emplace_back(mTextureHandle);
		if (mBackFaceTextureHandle != 0)
			staleTexturesList.emplace_back(mBackFaceTextureHandle);
	}
}

bool Volume3d::hasClipPlane()
{
	// return !__isinf(mClipEq[3]);
	return (mClipEq[3] < std::numeric_limits<float>::max());
}

void Volume3d::setClipPlane(float a, float b, float c, float d)
{
	mClipEq[0] = a;
	mClipEq[1] = b;
	mClipEq[2] = c;
	mClipEq[3] = d;
}

void Volume3d::clearClipPlane()
{
	mClipEq[0] = mClipEq[1] = mClipEq[2] = 0;
	// mClipEq[3] = -std::numeric_limits<float>::infinity();
	mClipEq[3] = std::numeric_limits<float>::max();
}

// (GL context)
void Volume3d::bind()
{
	CHECK(mMaterial->bind());
	CHECK(mBackFaceMaterial->bind());

	mVertexAttrib = mMaterial->getAttribute("vertex");
	mModelViewUniform = mMaterial->getUniform("modelView");
	mProjectionUniform = mMaterial->getUniform("projection");
	mDimensionsUniform = mMaterial->getUniform("dimensions");
	mClipPlaneUniform = mMaterial->getUniform("clipPlane");
	mSpacingUniform = mMaterial->getUniform("spacing");
	// mScreenSizeUniform = mMaterial->getUniform("screenSize");

	mBFVertexAttrib = mBackFaceMaterial->getAttribute("vertex");
	mBFModelViewUniform = mBackFaceMaterial->getUniform("modelView");
	mBFProjectionUniform = mBackFaceMaterial->getUniform("projection");
	mBFDimensionsUniform = mBackFaceMaterial->getUniform("dimensions");
	mBFSpacingUniform = mBackFaceMaterial->getUniform("spacing");

	android_assert(mVertexAttrib != -1);
	android_assert(mModelViewUniform != -1);
	android_assert(mProjectionUniform != -1);
	android_assert(mDimensionsUniform != -1);
	android_assert(mClipPlaneUniform != -1);
	android_assert(mSpacingUniform != -1);
	// android_assert(mScreenSizeUniform != -1);
	android_assert(mBFVertexAttrib != -1);
	android_assert(mBFModelViewUniform != -1);
	android_assert(mBFProjectionUniform != -1);
	android_assert(mBFDimensionsUniform != -1);
	android_assert(mBFSpacingUniform != -1);

	// checkError("get{Attribute/Uniform}");

	// Required because input is not RGBA (i.e. not aligned to a 4-byte boundary)
	// http://www.opengl.org/wiki/Common_Mistakes#Texture_upload_and_pixel_reads
	CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

	CHECK(glGenTextures(1, &mTextureHandle));
	CHECK(glBindTexture(GL_TEXTURE_3D/*_OES*/, mTextureHandle));
	CHECK(glTexParameteri(GL_TEXTURE_3D/*_OES*/, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	CHECK(glTexParameteri(GL_TEXTURE_3D/*_OES*/, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	CHECK(glTexParameteri(GL_TEXTURE_3D/*_OES*/, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	CHECK(glTexParameteri(GL_TEXTURE_3D/*_OES*/, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	CHECK(glTexParameteri(GL_TEXTURE_3D/*_OES*/, GL_TEXTURE_WRAP_R/*_OES*/, GL_CLAMP_TO_EDGE));

	// PFNGLTEXIMAGE3DOESPROC glTexImage3DOES = nullptr;
	// glTexImage3DOES = GetProcAddress<PFNGLTEXIMAGE3DOESPROC>("glTexImage3DOES");

	// Initialize the texture
	// CHECK(glTexImage3DOES(
	CHECK(glTexImage3D(
		GL_TEXTURE_3D/*_OES*/,
		0,
		GL_RGBA,
		mDimensions[0], mDimensions[1], mDimensions[2],
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		mTexture.data()
	));

	// Init the backface texture
	CHECK(glGenTextures(1, &mBackFaceTextureHandle));
	CHECK(glBindTexture(GL_TEXTURE_2D, mBackFaceTextureHandle));
	CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
	CHECK(glBindTexture(GL_TEXTURE_2D, 0));

    // // Create a depth framebuffer
    // GLuint depthBuffer;
    // glGenRenderbuffers(1, &depthBuffer);
    // glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    // glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
	// glBindRenderbuffer(GL_RENDERBUFFER, 0);

	CHECK(glGenFramebuffers(1, &mFrameBuffer));
	CHECK(glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer));
	CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mBackFaceTextureHandle, 0));
    // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (result != GL_FRAMEBUFFER_COMPLETE) {
	    throw std::runtime_error("Unable to create the depth framebuffer: error " + Utility::toString(result));
    }
    CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    static const GLfloat vertices[24] = {
	    0.0, 0.0, 0.0,
	    0.0, 0.0, 1.0,
	    0.0, 1.0, 0.0,
	    0.0, 1.0, 1.0,
	    1.0, 0.0, 0.0,
	    1.0, 0.0, 1.0,
	    1.0, 1.0, 0.0,
	    1.0, 1.0, 1.0
    };
    // draw the six faces of the boundbox by drawwing triangles
    // draw it contra-clockwise
    // front: 1 5 7 3
    // back: 0 2 6 4
    // left：0 1 3 2
    // right:7 5 4 6
    // up: 2 3 7 6
    // down: 1 0 4 5
    static const GLushort indices[36] = {
        1, 5, 7,
        7, 3, 1,
        0, 2, 6,
        6, 4, 0,
        0, 1, 3,
        3, 2, 0,
        7, 5, 4,
        4, 6, 7,
        2, 3, 7,
        7, 6, 2,
        1, 0, 4,
        4, 5, 1
    };

	// Allocate 2 VBOs
	GLuint vbos[2];
	glGenBuffers(2, vbos);

	mVertexBuffer = vbos[0];
	CHECK(glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer));
	CHECK(glBufferData(GL_ARRAY_BUFFER, 24*sizeof(GLfloat), vertices, GL_STATIC_DRAW));
	CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

	mIndexBuffer = vbos[1];
	CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer));
	CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36*sizeof(GLushort), indices, GL_STATIC_DRAW));
	CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

	mBound = true;
}

// (GL context)
void Volume3d::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	synchronized (staleTexturesList) {
		if (!staleTexturesList.empty()) {
			LOGD("freeing %ld texture(s)", staleTexturesList.size());
			for (GLuint texture : staleTexturesList)
				CHECK(glDeleteTextures(1, &texture));
			staleTexturesList.clear();
		}
	}

	if (!mBound)
		bind();

	android_assert(mMaterial);

	// Uniforms
	CHECK(glUseProgram(mMaterial->getHandle()));
	CHECK(glUniformMatrix4fv(mProjectionUniform, 1, false, projectionMatrix.data_));
	// CHECK(glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_));
	CHECK(glUniformMatrix4fv(mModelViewUniform, 1, false, (modelViewMatrix * Matrix4::makeTransform(-Vector3(mDimensions[0]*mSpacing.x, mDimensions[1]*mSpacing.y, -mDimensions[2]*mSpacing.z)/2)).data_));
	CHECK(glUniform4fv(mClipPlaneUniform, 1, mClipEq));
	CHECK(glUniform3f(mSpacingUniform, mSpacing.x, mSpacing.y, mSpacing.z));
	CHECK(glUniform3i(mDimensionsUniform, mDimensions[0], mDimensions[1], mDimensions[2]));
	// CHECK(glUniform2f(mScreenSizeUniform, screenWidth, screenHeight));

	// Uniforms 2
	CHECK(glUseProgram(mBackFaceMaterial->getHandle()));
	CHECK(glUniformMatrix4fv(mBFProjectionUniform, 1, false, projectionMatrix.data_));
	// CHECK(glUniformMatrix4fv(mBFModelViewUniform, 1, false, modelViewMatrix.data_));
	CHECK(glUniformMatrix4fv(mBFModelViewUniform, 1, false, (modelViewMatrix * Matrix4::makeTransform(-Vector3(mDimensions[0]*mSpacing.x, mDimensions[1]*mSpacing.y, -mDimensions[2]*mSpacing.z)/2)).data_));
	CHECK(glUniform3f(mBFSpacingUniform, mSpacing.x, mSpacing.y, mSpacing.z));
	CHECK(glUniform3i(mBFDimensionsUniform, mDimensions[0], mDimensions[1], mDimensions[2]));

	CHECK(glActiveTexture(GL_TEXTURE0));
	CHECK(glBindTexture(GL_TEXTURE_3D/*_OES*/, mTextureHandle));

	CHECK(glUseProgram(mMaterial->getHandle()));
	GLint textureSampler = mMaterial->getUniform("texture");
	android_assert(textureSampler != -1);
	CHECK(glUniform1i(textureSampler, 0));

	CHECK(glActiveTexture(GL_TEXTURE1));
	CHECK(glBindTexture(GL_TEXTURE_2D, mBackFaceTextureHandle));

	CHECK(glUseProgram(mMaterial->getHandle()));
	GLint backFaceTextureSampler = mMaterial->getUniform("backFaceTexture");
	android_assert(backFaceTextureSampler != -1);
	CHECK(glUniform1i(backFaceTextureSampler, 1));

	// Vertices
	android_assert(mVertexBuffer != 0);
	CHECK(glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer));
	android_assert(mVertexAttrib != -1);
	CHECK(glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false, 0, nullptr));
	CHECK(glEnableVertexAttribArray(mVertexAttrib));

	// Indices
	android_assert(mIndexBuffer != 0);
	CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer));

	// Render the back texture
	CHECK(glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer));
	CHECK(glUseProgram(mBackFaceMaterial->getHandle()));
	CHECK(glClearColor(0, 0, 0, 1));
	CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	CHECK(glEnable(GL_CULL_FACE));
	CHECK(glCullFace(GL_FRONT));
	CHECK(glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr));
	CHECK(glDisable(GL_CULL_FACE));
	CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	// Render the volume
	CHECK(glUseProgram(mMaterial->getHandle()));
	CHECK(glEnable(GL_CULL_FACE));
	CHECK(glCullFace(GL_BACK));
	CHECK(glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr)); // XXX: error here

	CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
	CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

	CHECK(glDisableVertexAttribArray(mVertexAttrib));
}
