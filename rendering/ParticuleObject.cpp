#include "rendering/ParticuleObject.h"
#include <list>
#include <limits>
#include "rendering/material.h"
#include "getprocaddress.h"

namespace
{
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

		"uniform highp vec4 clipPlane;\n"
		"varying highp float v_clipDist;\n"

		"void main() {\n"
		"  mediump vec3 scale = vec3(1.0, 1.0, 1.0);\n"
		"  highp vec4 viewSpacePos = modelView * vec4(scale * (vertex * vec3(1.0, 1.0, -1.0)), 1.0);\n"

		"  gl_Position = projection * viewSpacePos;\n"

		"  v_clipDist = dot(viewSpacePos.xyz, clipPlane.xyz) + clipPlane.w;\n"
		"}";

	const char* fragmentShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"//#extension GL_OES_texture_3D : require\n"
		"uniform highp mat4 projection;\n"
		"uniform highp mat4 modelView;\n"
		"uniform highp vec4 uColor;\n"
		"varying highp float v_clipDist;\n"

		"void main() {\n"
		"if (v_clipDist <= 0.0) discard;\n"
		"  gl_FragColor = uColor;\n"
		"}";
}

ParticuleObject::ParticuleObject(const std::string& fileStats, const std::string& fileData) : mMaterial(new Material(vertexShader, fragmentShader)), mBound(false)
{
	mColor[0] = 1.0f;
	mColor[1] = 0.0f;
	mColor[2] = 0.0f;
	mColor[3] = 1.0f;
	FILE* fdStats = fopen(fileStats.c_str(), "r");

	//Get the number of particules
	fseek(fdStats, 0, SEEK_END);
	uint32_t mNbParticules = ftell(fdStats) / sizeof(int);
	rewind(fdStats);

	FILE* fdPoints = fopen(fileData.c_str(), "r");

	//Init the array
	mPoints = (float*)malloc(sizeof(float)*3*mNbParticules);
	mPointsStats = (char*)malloc(sizeof(char)*mNbParticules);

	fread(mPoints, sizeof(float), 3*mNbParticules, fdPoints);
	fread(mPointsStats, sizeof(int), mNbParticules, fdStats);

	fclose(fdPoints);
	fclose(fdStats);

	mMin.x = std::numeric_limits<float>::max();
	mMin.y = std::numeric_limits<float>::max();
	mMin.z = std::numeric_limits<float>::max();

	mMax.x = std::numeric_limits<float>::min();
	mMax.y = std::numeric_limits<float>::min();
	mMax.z = std::numeric_limits<float>::min();

	for(uint32_t i=0; i < mNbParticules*3; i+=3)
	{
		if(mMin.x > mPoints[i]) mMin.x = mPoints[i];
		if(mMax.x < mPoints[i]) mMax.x = mPoints[i];
		if(mMin.y > mPoints[i+1]) mMin.y = mPoints[i+1];
		if(mMax.x < mPoints[i+1]) mMax.x = mPoints[i+1];
		if(mMin.z > mPoints[i+2]) mMin.z = mPoints[i+2];
		if(mMax.x < mPoints[i+2]) mMax.x = mPoints[i+2];
	}

	clearClipPlane();
}

ParticuleObject::~ParticuleObject()
{
	free(mPoints);
	free(mPointsStats);
}

bool ParticuleObject::hasClipPlane()
{
	// return !__isinf(mClipEq[3]);
	return (mClipEq[3] < std::numeric_limits<float>::max());
}

void ParticuleObject::setClipPlane(float a, float b, float c, float d)
{
	mClipEq[0] = a;
	mClipEq[1] = b;
	mClipEq[2] = c;
	mClipEq[3] = d;
}

void ParticuleObject::clearClipPlane()
{
	mClipEq[0] = mClipEq[1] = mClipEq[2] = 0;
	mClipEq[3] = std::numeric_limits<float>::max();
}

void ParticuleObject::bind()
{
	mMaterial->bind();

	mVertexAttrib = mMaterial->getAttribute("vertex");
	mSliceAttrib = mMaterial->getUniform("clipPlane");
	mProjectionUniform = mMaterial->getUniform("projection");
	mModelViewUniform = mMaterial->getUniform("mModelViewUniform");
	mColorUniform = mMaterial->getUniform("color");
}

void ParticuleObject::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	if(!mBound)
		bind();

	glUseProgram(mMaterial->getHandle());

	//Vertices
	glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false, 0, mPoints);
	glEnableVertexAttribArray(mVertexAttrib);

	//Uniform
	glUniformMatrix4fv(mProjectionUniform, 1, false, projectionMatrix.data_);
	glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
	glUniform4fv(mColorUniform, 1, mColor);
	glUniform4fv(mSliceAttrib, 1, mClipEq);

	glPointSize(2.0f);
	glDrawArrays(GL_POINTS, 0, mNbParticules);

	glUseProgram(0);
}
