#include "isosurface.h"

#include "rendering/material.h"

#include <limits>
#include <climits>

#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkTriangleFilter.h>
#include <vtkMarchingCubes.h>
#include <vtkDecimatePro.h>
#include <vtkPolyDataNormals.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>

namespace {
	const char* vertexShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform highp mat4 projection;\n"
		"uniform highp mat4 modelView;\n"
		"uniform mediump mat3 normalMatrix;\n"
		"uniform lowp ivec3 dimensions;\n"
		"attribute highp vec3 vertex;\n"
		"attribute mediump vec3 normal;\n"
		"varying mediump vec3 v_normal;\n"
		"varying lowp vec3 v_lightDir;\n"
		"varying lowp vec3 v_lightDir2;\n"

		"uniform highp vec4 clipPlane;\n"
		"varying highp float v_clipDist;\n"

		// "varying highp float v_depth;\n"

		"void main() {\n"
		"  v_normal = normalize(normalMatrix * normal*vec3(1.0, 1.0, -1.0));\n"
		// // FIXME: hardcoded scaling constant: 2.0
		// "  mediump vec3 scale = 2.0 * vec3(float(dimensions.x), float(dimensions.y), float(dimensions.z));\n"
		"  mediump vec3 scale = vec3(float(dimensions.x), float(dimensions.y), float(dimensions.z));\n"
		// "  gl_Position = projection * modelView * vec4(scale * (vertex * vec3(1.0, 1.0, -1.0) + vec3(0.0, 0.0, 0.5)), 1.0);\n"
		"  highp vec4 viewSpacePos = modelView * vec4(scale * (vertex * vec3(1.0, 1.0, 1.0)), 1.0);\n"
		"  gl_Position = projection * viewSpacePos;\n"

		// "  v_lightDir = normalize(vec3(-1.0, -2.0, 1.5));\n"
		// "  v_lightDir = normalize(vec3(1.0, 0.0, 0.0));\n" // from right
		// "  v_lightDir = normalize(vec3(0.0, 1.0, 0.0));\n" // from above (?)
		// "  v_lightDir = normalize(vec3(0.0, 0.0, 1.0));\n" // from front
		"  v_lightDir = normalize(vec3(1.2, -0.8, 0.4));\n"
		"  v_lightDir2 = normalize(vec3(-1.0, -0.3, -0.4));\n"

		// // "  v_depth = gl_Position.w;\n"
		// "  v_depth = viewSpacePos.w;\n"

		// "v_clipDist = dot(vertex.xyz, clipPlane.xyz) + clipPlane.w;\n"
		"v_clipDist = dot(viewSpacePos.xyz, clipPlane.xyz) + clipPlane.w;\n"
		// "v_clipDist = dot((modelView * vec4(scale*(vertex * vec3(1.0, 1.0, -1.0)), 1.0)).xyz, clipPlane.xyz) + clipPlane.w;\n"
		// "v_clipDist = dot(vec3(viewSpacePos.x, -viewSpacePos.z, viewSpacePos.y), clipPlane.xyz) + clipPlane.w;\n"

		"}"
		;

	const char* fragmentShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform lowp float value;\n"
		"uniform lowp float opacity;\n"
		"varying mediump vec3 v_normal;\n"
		"varying lowp vec3 v_lightDir;\n"
		"varying lowp vec3 v_lightDir2;\n"

		// "varying highp float v_depth;\n"
		"varying highp float v_clipDist;\n"

		// // "Jet" color map (http://www.metastine.com/?p=7)
		// // FIXME: duplicated code (see volume.cpp)
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

		// // NOTE: "discard" is required here to actually ignore the
		// // depth buffer value
		// // FIXME: hardcoded clipping value
		// "if (v_depth < 450.0) discard; else {\n"
		// // "if (v_depth < 450.0) gl_FragColor.a = 0.0; else {\n"
		// // "if (gl_FragCoord.z < 450.0) gl_FragColor.a = 0.0; else {\n"

		"if (v_clipDist > 0.0) discard;\n"

		// "  lowp vec3 lightDir = normalize(vec3(1.0, 1.0, -0.5));\n" // FIXME: hardcoded light direction
		"  lowp float NdotL = max(dot(normalize(v_normal), v_lightDir), 0.0);\n"
		"  lowp float NdotL2 = max(dot(normalize(v_normal), v_lightDir2), 0.0);\n"
		// "  lowp float NdotL_rev = max(dot(-normalize(v_normal), v_lightDir), 0.0);\n" // backface lighting
		// "  gl_FragColor = vec4(vec3(0.25+0.5*max(NdotL, NdotL_rev*0.5)), 1.0);\n"
		// "  gl_FragColor = vec4(colormap(value)*vec3(0.25+0.5*max(NdotL, NdotL_rev*0.5)), opacity);\n"
		// "  gl_FragColor = vec4(vec3(0.25+0.5*max(NdotL, NdotL_rev*0.5)), opacity);\n"
		// "  gl_FragColor = vec4(vec3(0.25+0.5*NdotL), opacity);\n"
		// "  gl_FragColor = vec4(vec3(0.25+0.6*max(NdotL, NdotL_rev*0.4)), opacity);\n"
		"  gl_FragColor = vec4(vec3(0.15+max(NdotL*0.65, NdotL2*0.35)), opacity);\n"
		"}"

		// "}"

		;
} // namespace

IsoSurface::IsoSurface(vtkSmartPointer<vtkImageData> data, bool stream)
 : mMaterial(MaterialSharedPtr(new Material(vertexShader, fragmentShader))),
   mData(data),
   mValue(0), mPendingValue(0),
   mBound(false), mIsEmpty(true), mDirty(false), mStream(stream),
   mVertexAttrib(-1), mNormalAttrib(-1),
   mProjectionUniform(-1), mModelViewUniform(-1), mNormalMatrixUniform(-1), mDimensionsUniform(-1), mValueUniform(-1), mOpacityUniform(-1), mClipPlaneUniform(-1),
   mVertexBuffer(0), mNormalBuffer(0), mIndexBuffer(0)
{
	android_assert(mData);

	clearClipPlane();

	// Check if the data dimension is 3
	int dim = mData->GetDataDimension();
	LOGD("dimension = %d", dim);
	if (dim != 3) {
		throw std::runtime_error(
			"IsoSurface: data is not 3D (dimension = " + Utility::toString(dim) + ")"
		);
	}

	if (!mData->GetPointData() || !mData->GetPointData()->GetScalars())
		throw std::runtime_error("IsoSurface: unsupported data");

	mData->GetDimensions(mDimensions);
	LOGD("dimensions %d %d %d", mDimensions[0], mDimensions[1], mDimensions[2]);
	mData->GetPointData()->GetScalars()->GetRange(mRange);
	LOGD("range min=%f max=%f", mRange[0], mRange[1]);
}

void IsoSurface::setValue(double value)
{
	// LOGD("computing surface for value: %f", value);

	if (value == mPendingValue) {
		// LOGD("surface value didn't change");
		return;
	}

	mVertices.clear();
	mNormals.clear();
	mIndices.clear();

	if (value < mRange[0] || value > mRange[1]) {
		// LOGD("surface value is outside bounds");
		mIsEmpty = true;
		return;
	}

	double datasetBounds[6], datasetCenter[3];
	mData->GetBounds(datasetBounds);
	mData->GetCenter(datasetCenter);

	// Triangulation filter
	vtkNew<vtkTriangleFilter> triangleFilter;

	// Filter out lines and single vertices
	triangleFilter->PassLinesOff();
	triangleFilter->PassVertsOff();

	if (!vtkPolyData::SafeDownCast(mData)) {
		vtkNew<vtkMarchingCubes> filter;
		filter->SetInputData(mData);
		filter->SetValue(0, value);
		filter->ComputeNormalsOff();
		triangleFilter->SetInputConnection(filter->GetOutputPort());
	} else {
		triangleFilter->SetInputData(mData);
	}

	// Triangulate mesh
	triangleFilter->Update();

	vtkPolyData* polyData = triangleFilter->GetOutput();
	android_assert(polyData);

	unsigned int vertexCount = polyData->GetNumberOfPoints();
	// LOGD("number of points = %u (limit: %u)", vertexCount, USHRT_MAX);

	if (vertexCount < 3) {
		// Not enough points for displaying at least one triangle...
		mIsEmpty = true;
		return;
	}

	vtkNew<vtkDecimatePro> decimateFilter;
	decimateFilter->SetInputData(polyData);
	decimateFilter->SetTargetReduction(0);
	decimateFilter->PreserveTopologyOff();
	decimateFilter->SplittingOn();
	decimateFilter->BoundaryVertexDeletionOn();
	decimateFilter->SetMaximumError(VTK_DOUBLE_MAX);
	decimateFilter->SetFeatureAngle(30.0);
	decimateFilter->SetSplitAngle(10.0);

	// FIXME: instead of decimation, create multiple meshes (i.e. multiple mIndices arrays)
	// and render all of them at the same time

	// OpenGL ES 2.0 only supports GL_UNSIGNED_SHORT as datatype
	// for indices, so the maximum number of vertices is USHRT_MAX
	// (unless the GL_OES_element_index_uint extension is available)

	if (vertexCount > USHRT_MAX) {
		LOGD("decimation required");

		// Try to decimate polyData
		double target = 1.0 - USHRT_MAX/double(vertexCount) + 0.1; // 0.1: safety margin
		do {
			if (target >= 1.0)
				throw std::runtime_error("IsoSurface: decimation failed, unable to render the surface");
			LOGD("decimation factor = %f", target);
			decimateFilter->SetTargetReduction(target);
			decimateFilter->Update();
			vertexCount = decimateFilter->GetOutput()->GetNumberOfPoints();
			LOGD("result: %u points", vertexCount);
			target += 0.1;
		} while (vertexCount > USHRT_MAX);

		LOGD("decimation finished (factor: %f)", target);
	}

	// TODO: triangle strips? (vtkStripper)

	if (!mStream) {
		vtkNew<vtkWindowedSincPolyDataFilter> smoothFilter;
		smoothFilter->SetInputConnection(decimateFilter->GetOutputPort());
		// smoothFilter->SetNumberOfIterations(15);
		smoothFilter->SetNumberOfIterations(10);
		smoothFilter->BoundarySmoothingOff();
		smoothFilter->FeatureEdgeSmoothingOff();
		smoothFilter->SetFeatureAngle(30.0);
		// smoothFilter->SetPassBand(0.001);
		smoothFilter->SetPassBand(0.1);
		smoothFilter->NonManifoldSmoothingOn();
		smoothFilter->NormalizeCoordinatesOn();

		smoothFilter->Update();
		polyData = decimateFilter->GetOutput();

	} else {
		decimateFilter->Update();
		polyData = decimateFilter->GetOutput();
	}

	android_assert(polyData);

	// Normalize the vertex coordinates, in order to match the
	// volume rendering
	vtkNew<vtkTransform> transform;
	transform->PostMultiply();
	transform->Translate(
		-datasetCenter[0],
		-datasetCenter[1],
		-datasetCenter[2]
	);
	transform->Scale(
		1.0 / mDimensions[0],
		1.0 / mDimensions[1],
		1.0 / mDimensions[2]
	);
	vtkNew<vtkTransformPolyDataFilter> transformFilter;
	transformFilter->SetInputData(polyData);
	transformFilter->SetTransform(transform.GetPointer());

	// Generate normals
	vtkNew<vtkPolyDataNormals> normalGenerator;
	normalGenerator->SetInputConnection(transformFilter->GetOutputPort());
	normalGenerator->ComputePointNormalsOn();
	normalGenerator->ComputeCellNormalsOff();
	normalGenerator->SetFeatureAngle(30.0);
	normalGenerator->SplittingOff();

	normalGenerator->Update();
	polyData = normalGenerator->GetOutput();

	// transformFilter->Update();
	// polyData = transformFilter->GetOutput();

	android_assert(polyData);
	android_assert(polyData->GetPointData());
	android_assert(polyData->GetPointData()->GetNormals());
	android_assert(polyData->GetPolys());

	// Read vertex positions
	mVertices.resize(vertexCount*3);
	for (unsigned int i = 0; i < vertexCount; ++i) {
		double* pt = polyData->GetPoint(i);
		mVertices[i*3+0] = pt[0];
		mVertices[i*3+1] = pt[1];
		mVertices[i*3+2] = pt[2];
		// LOGD("adding vertex: %f %f %f", pt[0], pt[1], pt[2]);
	}

	// Read vertex normals
	vtkDataArray* normals = polyData->GetPointData()->GetNormals();
	int normalCount = normals->GetNumberOfTuples();
	mNormals.resize(normalCount*3);
	for (int i = 0; i < normalCount; ++i) {
		double* n = normals->GetTuple(i);
		mNormals[i*3+0] = n[0];
		mNormals[i*3+1] = n[1];
		mNormals[i*3+2] = n[2];
		// LOGD("adding normal: %f %f %f", n[0], n[1], n[2]);
	}

	// Read indices
	vtkCellArray* polys = polyData->GetPolys();
	polys->InitTraversal();
	int indexCount = polys->GetNumberOfCells();
	mIndices.resize(indexCount*3);
	// LOGD("GetNumberOfCells() = %d", indexCount);
	for (int i = 0; i < indexCount; ++i) {
		vtkIdType numPts;
		vtkIdType* pts;
		polys->GetNextCell(numPts, pts);
		android_assert(numPts == 3);
		mIndices[i*3+0] = pts[0];
		mIndices[i*3+1] = pts[1];
		mIndices[i*3+2] = pts[2];
		// LOGD("adding indices: %u %u %u", pts[0], pts[1], pts[2]);
	}

	// LOGD("%d %d %d", mVertices.size(), mNormals.size(), mIndices.size());

	mPendingValue = value;
	mIsEmpty = false;

	mDirty = true;

	// LOGD("surface computed");
}

void IsoSurface::setPercentage(double value)
{
	android_assert(0.0 <= value && value <= 1.0);
	setValue(value*(mRange[1]-mRange[0]) + mRange[0]);
}

void IsoSurface::setClipPlane(float a, float b, float c, float d)
{
	mClipEq[0] = a;
	mClipEq[1] = b;
	mClipEq[2] = c;
	mClipEq[3] = d;
}

void IsoSurface::clearClipPlane()
{
	mClipEq[0] = mClipEq[1] = mClipEq[2] = 0;
	// mClipEq[3] = -std::numeric_limits<float>::infinity();
	mClipEq[3] = std::numeric_limits<float>::lowest();
}

// (GL context)
void IsoSurface::bind()
{
	if (mIsEmpty)
		return;

	mMaterial->bind();

	mVertexAttrib = mMaterial->getAttribute("vertex");
	mNormalAttrib = mMaterial->getAttribute("normal");
	mModelViewUniform = mMaterial->getUniform("modelView");
	mProjectionUniform = mMaterial->getUniform("projection");
	mNormalMatrixUniform = mMaterial->getUniform("normalMatrix");
	mDimensionsUniform = mMaterial->getUniform("dimensions");
	mValueUniform = mMaterial->getUniform("value");
	mOpacityUniform = mMaterial->getUniform("opacity");
	mClipPlaneUniform = mMaterial->getUniform("clipPlane");

	android_assert(mVertexAttrib != -1);
	android_assert(mNormalAttrib != -1);
	android_assert(mModelViewUniform != -1);
	android_assert(mProjectionUniform != -1);
	android_assert(mNormalMatrixUniform != -1);
	android_assert(mDimensionsUniform != -1);
	// android_assert(mValueUniform != -1);
	android_assert(mOpacityUniform != -1);
	android_assert(mClipPlaneUniform != -1);

	// Allocate 3 VBOs
	GLuint vbos[3];
	glGenBuffers(3, vbos);
	mVertexBuffer = vbos[0];
	mNormalBuffer = vbos[1];
	mIndexBuffer  = vbos[2];

	// // Vertices
	// glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	// LOGD("vertex size = %d", mVertices.size());
	// glBufferData(GL_ARRAY_BUFFER, USHRT_MAX*sizeof(GLfloat), nullptr, GL_STATIC_DRAW);
	// glBindBuffer(GL_ARRAY_BUFFER, 0);

	// // Normals
	// glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffer);
	// LOGD("normal size = %d", mNormals.size());
	// glBufferData(GL_ARRAY_BUFFER, USHRT_MAX*sizeof(GLfloat), nullptr, GL_STATIC_DRAW);
	// glBindBuffer(GL_ARRAY_BUFFER, 0);

	// // Indices
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	// LOGD("index size = %d", mIndices.size());
	// glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size()*sizeof(GLushort), nullptr, GL_STATIC_DRAW);
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	update();

	mBound = true;
}

// (GL context)
void IsoSurface::update()
{
	mValue = mPendingValue;

	const GLenum type = (mStream ? GL_STATIC_DRAW : GL_STREAM_DRAW);

	// Vertices
	android_assert(mVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, mVertices.size()*sizeof(GLfloat), nullptr, type);
	glBufferSubData(GL_ARRAY_BUFFER, 0, mVertices.size()*sizeof(GLfloat), mVertices.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Normals
	android_assert(mNormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, mNormals.size()*sizeof(GLfloat), nullptr, type);
	glBufferSubData(GL_ARRAY_BUFFER, 0, mNormals.size()*sizeof(GLfloat), mNormals.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Indices
	android_assert(mIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size()*sizeof(GLushort), nullptr, type);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, mIndices.size()*sizeof(GLushort), mIndices.data());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	mDirty = false;
}

// (GL context)
void IsoSurface::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	if (mIsEmpty)
		return;

	if (!mBound)
		bind();

	if (mDirty)
		update();

	// Vertices
	android_assert(mVertexAttrib != -1);
	glEnableVertexAttribArray(mVertexAttrib);
	android_assert(mVertexBuffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false, 0, nullptr);

	// Normals
	android_assert(mNormalAttrib != -1);
	glEnableVertexAttribArray(mNormalAttrib);
	android_assert(mNormalBuffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffer);
	glVertexAttribPointer(mNormalAttrib, 3, GL_FLOAT, false, 0, nullptr);

	// Indices
	android_assert(mIndexBuffer != 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

	// Rendering

	glUseProgram(mMaterial->getHandle());
	glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
	glUniformMatrix4fv(mProjectionUniform, 1, false, projectionMatrix.data_);
	glUniformMatrix3fv(mNormalMatrixUniform, 1, false, modelViewMatrix.inverse().transpose().get3x3Matrix().data_);
	glUniform3iv(mDimensionsUniform, 1, mDimensions);
	if (mValueUniform != -1) glUniform1f(mValueUniform, (mValue-mRange[0])/(mRange[1]-mRange[0]));
	// glUniform1f(mOpacityUniform, (mStream ? 0.5f : 1.0f));
	glUniform1f(mOpacityUniform, 1.0f);
	glUniform4fv(mClipPlaneUniform, 1, mClipEq);

	glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_SHORT, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(mVertexAttrib);
	glDisableVertexAttribArray(mNormalAttrib);
}
