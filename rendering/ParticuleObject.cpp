#include "rendering/ParticuleObject.h"
#include <list>
#include <limits>
#include "rendering/material.h"
#include "getprocaddress.h"
#include <sys/stat.h>

namespace
{
	const char* vertexShader =
		"#version 130\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform highp mat4 projection;\n"
		"uniform highp mat4 modelView;\n"
		"uniform lowp vec3 dimensions;\n" // (dimX,dimY,dimZ)
		"attribute highp vec3 vertex;\n"
		"attribute int  status;\n"

		"uniform highp vec4 clipPlane;\n"
		"varying highp float v_clipDist;\n"
		"varying highp vec4 v_color;\n"

		"void main() {\n"
		"  highp vec4 viewSpacePos = modelView * vec4(vertex * vec3(1.0, 1.0, -1.0), 1.0);\n"

		"  gl_Position = projection * viewSpacePos;\n"
		"  if(status == 0) v_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
		"  else if(status == 1) v_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
		"  else if(status == 2){v_color = vec4(0.0, 0.0, 1.0, 1.0);}\n"
//		"  gl_Size = 2.0;\n"

		"  v_clipDist = dot(viewSpacePos.xyz, clipPlane.xyz) + clipPlane.w;\n"
		"}";

	const char* fragmentShader =
		"#version 130\n"
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
		"varying highp vec4 v_color;\n"

		"void main() {\n"
		"if (v_clipDist <= 0.0) discard;\n"
	//	"if(gl_Color.b > 0.5) discard;\n"
		"gl_FragColor = v_color;\n"
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
	FILE* fdPoints = fopen(fileData.c_str(), "r");

	struct stat st1; 
	struct stat st2;

    stat(fileStats.c_str(), &st1);
	stat(fileData.c_str(), &st2);
	mNbParticules = fmin(st1.st_size, st2.st_size);

	//Init the array
	mPoints = (float*)malloc(sizeof(float)*3*mNbParticules);
	mPointsStats = (int*)malloc(sizeof(int)*mNbParticules);

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
		if(mMax.y < mPoints[i+1]) mMax.y = mPoints[i+1];
		if(mMin.z > mPoints[i+2]) mMin.z = mPoints[i+2];
		if(mMax.z < mPoints[i+2]) mMax.z = mPoints[i+2];
	}

	Vector3 mid = getMiddle();
	for(uint32_t i=0; i < mNbParticules*3; i+=3)
	{
		mPoints[i]-=mid.x;
		mPoints[i+1]-=mid.y;
		mPoints[i+2]-=mid.z;
	}

	mMin -= mid;
	mMax -= mid;
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
	mModelViewUniform = mMaterial->getUniform("modelView");
	mColorUniform = mMaterial->getUniform("uColor");
	mDimensionsUniform = mMaterial->getUniform("dimensions");
	mStatusAttrib = mMaterial->getAttribute("status");

	mBound = true;
}

void ParticuleObject::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	if(!mBound)
		bind();

	glUseProgram(mMaterial->getHandle());
	glEnableVertexAttribArray(mVertexAttrib);
	glEnableVertexAttribArray(mStatusAttrib);
	glPointSize(1.0f);

	//Vertices
	glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false, 0, mPoints);
	glVertexAttribIPointer(mStatusAttrib, 1, GL_INT, 0, mPointsStats);

	//Uniform
	glUniformMatrix4fv(mProjectionUniform, 1, false, projectionMatrix.data_);
	glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
	glUniform4fv(mColorUniform, 1, mColor);
	glUniform4fv(mSliceAttrib, 1, mClipEq);
	glUniform3f(mDimensionsUniform, getSize().x, getSize().y, getSize().z);


	for(uint32_t i=0; i < mNbParticules; i+=1000)
	{
		glDrawArrays(GL_POINTS, i, fmin(1000, (mNbParticules-i)));
	}
	glDisableVertexAttribArray(mVertexAttrib);
	glDisableVertexAttribArray(mStatusAttrib);
	glUseProgram(0);
}

void ParticuleObject::getStats(ParticuleStats* ps, FillVolume* fv)
{
	memset(ps, 0x00, sizeof(ps));
	ps->nbParticule = mNbParticules;
	for(uint32_t i=0; i < mNbParticules; i++)
	{
		Vector3 size = getSize();
		if(fv->get((size.x/2.0 + mPoints[3*i])*METRICS, (size.y/2.0 + mPoints[3*i+1])*METRICS, (size.z/2.0 + mPoints[3*i+2])*METRICS))
		{
			switch(mPointsStats[i])
			{
				case 0:
					ps->valid++;
					break;

				case 1:
					ps->incorrect++;
					break;

				case 2:
					ps->inNoise++;
					break;
			}
			ps->volume++;
		}
	}
}
