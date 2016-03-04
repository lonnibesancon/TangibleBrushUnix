#include "fluids_app.h"

#include "vtk_output_window.h"
#include "vtk_error_observer.h"
#include "volume.h"
#include "volume3d.h"
#include "isosurface.h"
#include "slice.h"
#include "rendering/cube.h"
#include "loaders/loader_obj.h"
#include "rendering/mesh.h"
#include "rendering/lines.h"

#include <array>
#include <map>
#include <time.h>

#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkDataSetReader.h>
#include <vtkXMLImageDataReader.h>
#include <vtkImageData.h>
#include <vtkImageResize.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkProbeFilter.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#define NEW_STYLUS_RENDER

struct Particle
{
	Vector3 pos;
	bool valid;
	int delayMs, stallMs;
	timespec lastTime;
};

struct FluidMechanics::Impl
{
	Impl(const std::string& baseDir);

	bool loadDataSet(const std::string& fileName);
	bool loadVelocityDataSet(const std::string& fileName);

	template <typename T>
	vtkSmartPointer<vtkImageData> loadTypedDataSet(const std::string& fileName);

	// (GL context)
	void rebind();

	// (GL context)
	void renderObjects();

	void setMatrices(const Matrix4& volumeMatrix, const Matrix4& stylusMatrix);
	void updateSlicePlanes();

	void buttonPressed();
	float buttonReleased();

	void releaseParticles();
	Vector3 particleJitter();
	void integrateParticleMotion(Particle& p);

	bool computeCameraClipPlane(Vector3& point, Vector3& normal);
	bool computeAxisClipPlane(Vector3& point, Vector3& normal);
	bool computeStylusClipPlane(Vector3& point, Vector3& normal);

	Vector3 posToDataCoords(const Vector3& pos); // "pos" is in eye coordinates
	Vector3 dataCoordsToPos(const Vector3& dataCoordsToPos);

	void updateSurfacePreview();

	FluidMechanics* app;
	SettingsPtr settings;
	StatePtr state;

	CubePtr cube, axisCube;

	vtkSmartPointer<vtkImageData> data, dataLow;
	int dataDim[3];
	Vector3 dataSpacing;

	vtkSmartPointer<vtkImageData> velocityData;

	typedef LinearMath::Vector3<int> DataCoords;
	// std::array<Particle, 10> particles;
	// std::array<Particle, 50> particles;
	// std::array<Particle, 100> particles;
	Synchronized<std::array<Particle, 200>> particles;
	timespec particleStartTime;
	static constexpr float particleSpeed = 0.15f;
	// static constexpr int particleReleaseDuration = 500; // ms
	static constexpr int particleReleaseDuration = 700; // ms
	static constexpr int particleStallDuration = 1000; // ms

	// static constexpr float stylusEffectorDist = 20.0f;
	static constexpr float stylusEffectorDist = 24.0f;
	// static constexpr float stylusEffectorDist = 30.0f;

	Synchronized<VolumePtr> volume;
	// Synchronized<Volume3dPtr> volume; // true volumetric rendering
	Synchronized<IsoSurfacePtr> isosurface, isosurfaceLow;
	Synchronized<SlicePtr> slice;
	Synchronized<CubePtr> outline;
	Vector3 slicePoint, sliceNormal;
	float sliceDepth;
	Synchronized<std::vector<Vector3>> slicePoints; // max size == 6

	MeshPtr particleSphere, cylinder;
	LinesPtr lines;

	vtkSmartPointer<vtkProbeFilter> probeFilter;

	Synchronized<Vector3> effectorIntersection;
	// Vector3 effectorIntersectionNormal;
	bool effectorIntersectionValid;

	bool buttonIsPressed;
};

FluidMechanics::Impl::Impl(const std::string& baseDir)
 : buttonIsPressed(false)
{
	cube.reset(new Cube);
	axisCube.reset(new Cube(true));
	particleSphere = LoaderOBJ::load(baseDir + "/sphere.obj");
	cylinder = LoaderOBJ::load(baseDir + "/cylinder.obj");
	lines.reset(new Lines);

	for (Particle& p : particles)
		p.valid = false;
}

void FluidMechanics::Impl::rebind()
{
	cube->bind();
	axisCube->bind();
	lines->bind();
	particleSphere->bind();
	cylinder->bind();

	synchronized_if(volume) { volume->bind(); }
	synchronized_if(isosurface) { isosurface->bind(); }
	synchronized_if(isosurfaceLow) { isosurfaceLow->bind(); }
	synchronized_if(slice) { slice->bind(); }
	synchronized_if(outline) { outline->bind(); }
}

template <typename T>
vtkSmartPointer<vtkImageData> FluidMechanics::Impl::loadTypedDataSet(const std::string& fileName)
{
	vtkNew<T> reader;

	LOGI("Loading file: %s...", fileName.c_str());
	reader->SetFileName(fileName.c_str());

	vtkNew<VTKErrorObserver> errorObserver;
	reader->AddObserver(vtkCommand::ErrorEvent, errorObserver.GetPointer());

	reader->Update();

	if (errorObserver->hasError()) {
		// TODO? Throw a different type of error to let Java code
		// display a helpful message to the user
		throw std::runtime_error("Error loading data: " + errorObserver->getErrorMessage());
	}

	vtkSmartPointer<vtkImageData> data = vtkSmartPointer<vtkImageData>::New();
	data->DeepCopy(reader->GetOutputDataObject(0));

	return data;
}

bool FluidMechanics::Impl::loadDataSet(const std::string& fileName)
{
	// // Unload mesh data
	// mesh.reset();

	synchronized (particles) {
		// Unload velocity data
		velocityData = nullptr;

		// Delete particles
		for (Particle& p : particles)
			p.valid = false;
	}

	VTKOutputWindow::install();

	const std::string ext = fileName.substr(fileName.find_last_of(".") + 1);

	if (ext == "vtk")
		data = loadTypedDataSet<vtkDataSetReader>(fileName);
	else if (ext == "vti")
		data = loadTypedDataSet<vtkXMLImageDataReader>(fileName);
	else
		throw std::runtime_error("Error loading data: unknown extension: \"" + ext + "\"");

	data->GetDimensions(dataDim);

	double spacing[3];
	data->GetSpacing(spacing);
	dataSpacing = Vector3(spacing[0], spacing[1], spacing[2]);

	// Compute a default zoom value according to the data dimensions
	// static const float nativeSize = 128.0f;
	static const float nativeSize = 110.0f;
	state->computedZoomFactor = nativeSize / std::max(dataSpacing.x*dataDim[0], std::max(dataSpacing.y*dataDim[1], dataSpacing.z*dataDim[2]));
	// FIXME: hardcoded value: 0.25 (minimum zoom level, see the
	// onTouch() handler in Java code)
	state->computedZoomFactor = std::max(state->computedZoomFactor, 0.25f);

	dataLow = vtkSmartPointer<vtkImageData>::New();
	vtkNew<vtkImageResize> resizeFilter;
	resizeFilter->SetInputData(data.GetPointer());
	resizeFilter->SetOutputDimensions(std::max(dataDim[0]/3, 1), std::max(dataDim[1]/3, 1), std::max(dataDim[2]/3, 1));
	resizeFilter->InterpolateOn();
	resizeFilter->Update();
	dataLow->DeepCopy(resizeFilter->GetOutput());

	probeFilter = vtkSmartPointer<vtkProbeFilter>::New();
	probeFilter->SetSourceData(data.GetPointer());

	synchronized(outline) {
		LOGD("creating outline...");
		outline.reset(new Cube(true));
		outline->setScale(Vector3(dataDim[0]/2, dataDim[1]/2, dataDim[2]/2) * dataSpacing);
	}

	synchronized(volume) {
		LOGD("creating volume...");
		volume.reset(new Volume(data));
		// volume.reset(new Volume3d(data));
		// if (fileName.find("FTLE7.vtk") != std::string::npos) { // HACK
		// 	// volume->setOpacity(0.25f);
		// 	volume->setOpacity(0.15f);
		// }
	}

	if (fileName.find("FTLE7.vtk") == std::string::npos) { // HACK
		synchronized(isosurface) {
			LOGD("creating isosurface...");
			isosurface.reset(new IsoSurface(data));
			isosurface->setPercentage(settings->surfacePercentage);
		}

		synchronized(isosurfaceLow) {
			LOGD("creating low-res isosurface...");
			isosurfaceLow.reset(new IsoSurface(dataLow, true));
			isosurfaceLow->setPercentage(settings->surfacePercentage);
		}
	} else {
		isosurface.reset();
		isosurfaceLow.reset();
	}

	synchronized(slice) {
		LOGD("creating slice...");
		slice.reset(new Slice(data));
	}

	return true;
}

bool FluidMechanics::Impl::loadVelocityDataSet(const std::string& fileName)
{
	if (!data)
		throw std::runtime_error("No dataset currently loaded");

	VTKOutputWindow::install();

	const std::string ext = fileName.substr(fileName.find_last_of(".") + 1);

	if (ext == "vtk")
		velocityData = loadTypedDataSet<vtkDataSetReader>(fileName);
	else if (ext == "vti")
		velocityData = loadTypedDataSet<vtkXMLImageDataReader>(fileName);
	else
		throw std::runtime_error("Error loading data: unknown extension: \"" + ext + "\"");

	int velocityDataDim[3];
	velocityData->GetDimensions(velocityDataDim);

	if (velocityDataDim[0] != dataDim[0]
	    || velocityDataDim[1] != dataDim[1]
	    || velocityDataDim[2] != dataDim[2])
	{
		throw std::runtime_error(
			"Dimensions do not match: "
			"vel: " + Utility::toString(velocityDataDim[0]) + "x" + Utility::toString(velocityDataDim[1]) + "x" + Utility::toString(velocityDataDim[2])
			+ ", data: " + Utility::toString(dataDim[0]) + "x" + Utility::toString(dataDim[1]) + "x" + Utility::toString(dataDim[2])
		);
	}

	int dim = velocityData->GetDataDimension();
	if (dim != 3)
		throw std::runtime_error("Velocity data is not 3D (dimension = " + Utility::toString(dim) + ")");

	if (!velocityData->GetPointData() || !velocityData->GetPointData()->GetVectors())
		throw std::runtime_error("Invalid velocity data: no vectors found");

	return true;
}

Vector3 FluidMechanics::Impl::posToDataCoords(const Vector3& pos)
{
	Vector3 result;

	synchronized(state->modelMatrix) {
		// Transform "pos" into object space
		result = state->modelMatrix.inverse() * pos;
	}

	// Compensate for the scale factor
	result *= 1/settings->zoomFactor;

	// The data origin is on the corner, not the center
	result += Vector3(dataDim[0]/2, dataDim[1]/2, dataDim[2]/2) * dataSpacing;

	return result;
}

Vector3 FluidMechanics::Impl::particleJitter()
{
	return Vector3(
		(float(std::rand()) / RAND_MAX),
		(float(std::rand()) / RAND_MAX),
		(float(std::rand()) / RAND_MAX)
	) * 1.0f;
	// ) * 0.5f;
}

void FluidMechanics::Impl::buttonPressed()
{
	buttonIsPressed = true;
}

float FluidMechanics::Impl::buttonReleased()
{
	buttonIsPressed = false;

	settings->surfacePreview = false;
	try {
		updateSurfacePreview();
		return settings->surfacePercentage;
	} catch (const std::exception& e) {
		LOGD("Exception: %s", e.what());
		return 0.0f;
	}
}

void FluidMechanics::Impl::releaseParticles()
{
	if (!velocityData || !state->tangibleVisible || !state->stylusVisible)
		return;

	Matrix4 smm;
	synchronized (state->stylusModelMatrix) {
		smm = state->stylusModelMatrix;
	}

	const float size = 0.5f * (stylusEffectorDist + std::max(dataSpacing.x*dataDim[0], std::max(dataSpacing.y*dataDim[1], dataSpacing.z*dataDim[2])));
	Vector3 dataPos = posToDataCoords(smm * Matrix4::makeTransform(Vector3(-size, 0, 0)*settings->zoomFactor) * Vector3::zero());

	if (dataPos.x < 0 || dataPos.y < 0 || dataPos.z < 0
	    || dataPos.x >= dataDim[0] || dataPos.y >= dataDim[1] || dataPos.z >= dataDim[2])
	{
		LOGD("outside bounds");
		return;
	}

	DataCoords coords(dataPos.x, dataPos.y, dataPos.z);
	// LOGD("coords = %s", Utility::toString(coords).c_str());

	// vtkDataArray* vectors = velocityData->GetPointData()->GetVectors();
	// double* v = vectors->GetTuple3(coords.z*(dataDim[0]*dataDim[1]) + coords.y*dataDim[0] + coords.x);
	// LOGD("v = %f, %f, %f", v[0], v[1], v[2]);

	// TODO(?)
	// vtkNew<vtkStreamLine> streamLine;
	// streamLine->SetInputData(velocityData);
	// // streamLine->SetStartPosition(coords.x, coords.y, coords.z);
	// streamLine->SetStartPosition(dataPos.x, dataPos.y, dataPos.z);
	// streamLine->SetMaximumPropagationTime(200);
	// streamLine->SetIntegrationStepLength(.2);
	// streamLine->SetStepLength(.001);
	// streamLine->SetNumberOfThreads(1);
	// streamLine->SetIntegrationDirectionToForward();
	// streamLine->Update();
	// vtkDataArray* vectors = streamLine->GetPointData()->GetVectors();
	// android_assert(vectors);
	// unsigned int num = vectors->GetNumberOfTuples();
	// LOGD("num = %d", num);
	// for (unsigned int i = 0; i < num; ++j) {
	// 	double* v = vectors->GetTuple3(i);
	// 	Vector3 pos(v[0], v[1], v[2]);
	// 	LOGD("pos = %s", Utility::toString(pos).c_str());
	// }

	clock_gettime(CLOCK_REALTIME, &particleStartTime);

	int delay = 0;
	synchronized (particles) {
		for (Particle& p : particles) {
			p.pos = Vector3(coords.x, coords.y, coords.z) + particleJitter();
			p.lastTime = particleStartTime;
			p.delayMs = delay;
			delay += particleReleaseDuration/particles.size();
			p.stallMs = 0;
			p.valid = true;
		}
	}
}

void FluidMechanics::Impl::integrateParticleMotion(Particle& p)
{
	if (!p.valid)
		return;

	// Pause particle motion when the data is not visible
	if (!state->tangibleVisible)
		return;

	timespec now;
	clock_gettime(CLOCK_REALTIME, &now);

	int elapsedMs = (now.tv_sec - p.lastTime.tv_sec) * 1000
		+ (now.tv_nsec - p.lastTime.tv_nsec) / 1000000;

	p.lastTime = now;

	if (p.delayMs > 0) {
		p.delayMs -= elapsedMs;
		if (p.delayMs < 0)
			elapsedMs = -p.delayMs;
		else
			return;
	}

	if (p.stallMs > 0) {
		p.stallMs -= elapsedMs;
		if (p.stallMs < 0)
			p.valid = false;
		return;
	}

	vtkDataArray* vectors = velocityData->GetPointData()->GetVectors();

	while (elapsedMs > 0) {
		--elapsedMs;

		DataCoords coords = DataCoords(p.pos.x, p.pos.y, p.pos.z);

		if (coords.x < 0 || coords.y < 0 || coords.z < 0
		    || coords.x >= dataDim[0] || coords.y >= dataDim[1] || coords.z >= dataDim[2])
		{
			// LOGD("particle moved outside bounds");
			p.valid = false;
			return;
		}

		double* v = vectors->GetTuple3(coords.z*(dataDim[0]*dataDim[1]) + coords.y*dataDim[0] + coords.x);
		// LOGD("v = %f, %f, %f", v[0], v[1], v[2]);

		// Vector3 vel(v[0], v[1], v[2]);
		Vector3 vel(v[1], v[0], v[2]); // XXX: workaround for a wrong data orientation

		if (!vel.isNull()) {
			p.pos += vel * particleSpeed;
		} else {
			// LOGD("particle stopped");
			p.stallMs = particleStallDuration;
			break;
		}
	}
}

bool FluidMechanics::Impl::computeCameraClipPlane(Vector3& point, Vector3& normal)
{
	// static const float weight = 0.3f;
	// static const float weight = 0.5f;
	static const float weight = 0.8f;
	static bool wasVisible = false;
	static Vector3 prevPos;

	if (!state->tangibleVisible) {
		wasVisible = false;
		return false;
	}

	Matrix4 slicingMatrix;
	// synchronized(modelMatrix) { // not needed since this thread is the only one to write to "modelMatrix"
	// Compute the inverse rotation matrix to render this
	// slicing plane
	slicingMatrix = Matrix4((app->getProjMatrix() * state->modelMatrix).inverse().get3x3Matrix());
	// }

	// Compute the slicing origin location in data coordinates:

	// Center of the screen (at depth "clipDist")
	Vector3 screenSpacePos = Vector3(0, 0, settings->clipDist);

	// Transform the position in object space
	Vector3 pos = state->modelMatrix.inverse() * screenSpacePos;

	// Transform the screen normal in object space
	Vector3 n = (state->modelMatrix.transpose().get3x3Matrix() * Vector3::unitZ()).normalized();

	// Filter "pos" using a weighted average, but only in the
	// "n" direction (the screen direction)
	// TODO: Kalman filter?
	if (wasVisible)
		pos += -n.project(pos) + n.project(pos*weight + prevPos*(1-weight));
	wasVisible = true;
	prevPos = pos;

	// Transform the position back in screen space
	screenSpacePos = state->modelMatrix * pos;

	// Store the computed depth
	sliceDepth = screenSpacePos.z;

	// Unproject the center of the screen (at the computed depth
	// "sliceDepth"), then convert the result into data coordinates
	Vector3 pt = app->getProjMatrix().inverse() * Vector3(0, 0, app->getDepthValue(sliceDepth));
	Vector3 dataCoords = posToDataCoords(pt);
	slicingMatrix.setPosition(dataCoords);

	synchronized(slice) {
		slice->setSlice(slicingMatrix, sliceDepth, settings->zoomFactor);
	}

	point = pt;
	normal = -Vector3::unitZ();

	return true;
}

bool FluidMechanics::Impl::computeAxisClipPlane(Vector3& point, Vector3& normal)
{
	if (state->tangibleVisible) {
		Matrix3 normalMatrix = state->modelMatrix.inverse().transpose().get3x3Matrix();
		float xDot = (normalMatrix*Vector3::unitX()).normalized().dot(Vector3::unitZ());
		float yDot = (normalMatrix*Vector3::unitY()).normalized().dot(Vector3::unitZ());
		float zDot = (normalMatrix*Vector3::unitZ()).normalized().dot(Vector3::unitZ());
		// Prevent back and forth changes between two axis (unless no
		// axis is defined yet)
		const float margin = (state->clipAxis != CLIP_NONE ? 0.1f : 0.0f);
		if (std::abs(xDot) > std::abs(yDot)+margin && std::abs(xDot) > std::abs(zDot)+margin) {
			state->clipAxis = (xDot < 0 ? CLIP_AXIS_X : CLIP_NEG_AXIS_X);
		} else if (std::abs(yDot) > std::abs(xDot)+margin && std::abs(yDot) > std::abs(zDot)+margin) {
			state->clipAxis = (yDot < 0 ? CLIP_AXIS_Y : CLIP_NEG_AXIS_Y);
		} else if (std::abs(zDot) > std::abs(xDot)+margin && std::abs(zDot) > std::abs(yDot)+margin) {
			state->clipAxis = (zDot < 0 ? CLIP_AXIS_Z : CLIP_NEG_AXIS_Z);
		}

		if (state->lockedClipAxis != CLIP_NONE) {
			Vector3 axis;
			ClipAxis neg;
			switch (state->lockedClipAxis) {
				case CLIP_AXIS_X: axis = Vector3::unitX(); neg = CLIP_NEG_AXIS_X; break;
				case CLIP_AXIS_Y: axis = Vector3::unitY(); neg = CLIP_NEG_AXIS_Y; break;
				case CLIP_AXIS_Z: axis = Vector3::unitZ(); neg = CLIP_NEG_AXIS_Z; break;
				case CLIP_NEG_AXIS_X: axis = -Vector3::unitX(); neg = CLIP_AXIS_X; break;
				case CLIP_NEG_AXIS_Y: axis = -Vector3::unitY(); neg = CLIP_AXIS_Y; break;
				case CLIP_NEG_AXIS_Z: axis = -Vector3::unitZ(); neg = CLIP_AXIS_Z; break;
				default: android_assert(false);
			}
			float dot = (normalMatrix*axis).normalized().dot(Vector3::unitZ());
			if (dot > 0)
				state->lockedClipAxis = neg;
		}

	} else {
		state->clipAxis = state->lockedClipAxis = CLIP_NONE;
	}

	const ClipAxis ca = (state->lockedClipAxis != CLIP_NONE ? state->lockedClipAxis : state->clipAxis);

	if (ca == CLIP_NONE)
		return false;

	Vector3 axis;
	Quaternion rot;
	switch (ca) {
		case CLIP_AXIS_X: axis = Vector3::unitX(); rot = Quaternion(Vector3::unitY(), -M_PI/2)*Quaternion(Vector3::unitZ(), M_PI); break;
		case CLIP_AXIS_Y: axis = Vector3::unitY(); rot = Quaternion(Vector3::unitX(),  M_PI/2)*Quaternion(Vector3::unitZ(), M_PI); break;
		case CLIP_AXIS_Z: axis = Vector3::unitZ(); rot = Quaternion::identity(); break;
		case CLIP_NEG_AXIS_X: axis = -Vector3::unitX(); rot = Quaternion(Vector3::unitY(),  M_PI/2)*Quaternion(Vector3::unitZ(), M_PI); break;
		case CLIP_NEG_AXIS_Y: axis = -Vector3::unitY(); rot = Quaternion(Vector3::unitX(), -M_PI/2)*Quaternion(Vector3::unitZ(), M_PI); break;
		case CLIP_NEG_AXIS_Z: axis = -Vector3::unitZ(); rot = Quaternion(Vector3::unitX(),  M_PI); break;
		default: android_assert(false);
	}

	// Project "pt" on the chosen axis in object space
	Vector3 pt = state->modelMatrix.inverse() * app->getProjMatrix().inverse() * Vector3(0, 0, app->getDepthValue(settings->clipDist));
	Vector3 absAxis = Vector3(std::abs(axis.x), std::abs(axis.y), std::abs(axis.z));
	Vector3 pt2 = absAxis * absAxis.dot(pt);

	// Return to eye space
	pt2 = state->modelMatrix * pt2;

	Vector3 dataCoords = posToDataCoords(pt2);

	// static const float size = 128.0f;
	const float size = 0.5f * std::max(dataSpacing.x*dataDim[0], std::max(dataSpacing.y*dataDim[1], dataSpacing.z*dataDim[2]));

	Matrix4 proj = app->getProjMatrix(); proj[0][0] = -proj[1][1] / 1.0f; // same as "projMatrix", but with aspect = 1
	Matrix4 slicingMatrix = Matrix4((proj * Matrix4::makeTransform(dataCoords, rot)).inverse().get3x3Matrix());
	slicingMatrix.setPosition(dataCoords);
	synchronized(slice) {
		slice->setSlice(slicingMatrix, -proj[1][1]*size*settings->zoomFactor, settings->zoomFactor);
	}

	synchronized(state->sliceModelMatrix) {
		state->sliceModelMatrix = Matrix4(state->modelMatrix * Matrix4::makeTransform(state->modelMatrix.inverse() * pt2, rot, settings->zoomFactor*Vector3(size, size, 0.0f)));
	}

	if (!slice->isEmpty())
		state->lockedClipAxis = ca;
	else
		state->lockedClipAxis = CLIP_NONE;

	point = pt2;
	normal = state->modelMatrix.inverse().transpose().get3x3Matrix() * axis;

	return true;
}

bool FluidMechanics::Impl::computeStylusClipPlane(Vector3& point, Vector3& normal)
{
#if 0
	// static const float posWeight = 0.7f;
	// static const float rotWeight = 0.8f;
	static const float posWeight = 0.8f;
	static const float rotWeight = 0.8f;

	static bool wasVisible = false;
	static Matrix4 prevMatrix;

	if (!state->stylusVisible) {
		wasVisible = false;
		return false;
	}
#else
	if (!state->stylusVisible)
		return false;
#endif

	// FIXME: state->stylusModelMatrix may be invalid (non-invertible) in some cases
	try {

	// Vector3 pt = state->stylusModelMatrix * Vector3::zero();
	// LOGD("normal = %s", Utility::toString(normal).c_str());
	// LOGD("pt = %s", Utility::toString(pt).c_str());

	// static const float size = 128.0f;
	// static const float size = 180.0f;
	const float size = 0.5f * (60.0f + std::max(dataSpacing.x*dataDim[0], std::max(dataSpacing.y*dataDim[1], dataSpacing.z*dataDim[2])));

	Matrix4 planeMatrix = state->stylusModelMatrix;

	// Matrix4 planeMatrix = state->stylusModelMatrix;
	// // planeMatrix = planeMatrix * Matrix4::makeTransform(Vector3(-size, 0, 0)*settings->zoomFactor);

	// Project the stylus->data vector onto the stylus X axis
	Vector3 dataPosInStylusSpace = state->stylusModelMatrix.inverse() * state->modelMatrix * Vector3::zero();

	// Shift the clip plane along the stylus X axis in order to
	// reach the data, even if the stylus is far away
	// Vector3 offset = (-Vector3::unitX()).project(dataPosInStylusSpace);

	// Shift the clip plane along the other axis in order to keep
	// it centered on the data
	Vector3 n = Vector3::unitZ(); // plane normal in stylus space
	// Vector3 v = dataPosInStylusSpace - offset; // vector from the temporary position to the data center point
	// offset += v.planeProject(n); // project "v" on the plane, and shift the clip plane according to the result
	Vector3 v = dataPosInStylusSpace;
	Vector3 offset = v.projectOnPlane(n); // project "v" on the plane, and shift the clip plane according to the result

	// Apply the computed offset
	planeMatrix = planeMatrix * Matrix4::makeTransform(offset);

	// The slice will be rendered from the viewpoint of the plane
	Matrix4 proj = app->getProjMatrix(); proj[0][0] = -proj[1][1] / 1.0f; // same as "projMatrix", but with aspect = 1
	Matrix4 slicingMatrix = Matrix4((proj * planeMatrix.inverse() * state->modelMatrix).inverse().get3x3Matrix());

	Vector3 pt2 = planeMatrix * Vector3::zero();

	// Position of the stylus tip, in data coordinates
	Vector3 dataCoords = posToDataCoords(pt2);
	// LOGD("dataCoords = %s", Utility::toString(dataCoords).c_str());
	slicingMatrix.setPosition(dataCoords);

	synchronized(slice) {
		slice->setSlice(slicingMatrix, -proj[1][1]*size*settings->zoomFactor, settings->zoomFactor);
	}

	synchronized(state->sliceModelMatrix) {
		state->sliceModelMatrix = Matrix4(planeMatrix * Matrix4::makeTransform(Vector3::zero(), Quaternion::identity(), settings->zoomFactor*Vector3(size, size, 0.0f)));
	}

	point = pt2;
	normal = state->stylusModelMatrix.inverse().transpose().get3x3Matrix() * Vector3::unitZ();

	} catch (const std::exception& e) { LOGD("%s", e.what()); return false; }

	return true;
}

Vector3 FluidMechanics::Impl::dataCoordsToPos(const Vector3& dataCoords)
{
	Vector3 result = dataCoords;

	// The data origin is on the corner, not the center
	result -= Vector3(dataDim[0]/2, dataDim[1]/2, dataDim[2]/2) * dataSpacing;

	// Compensate for the scale factor
	result *= settings->zoomFactor;

	synchronized(state->modelMatrix) {
		// Transform "result" into eye space
		result = state->modelMatrix * result;
	}

	return result;
}

template <typename T>
T lowPassFilter(const T& cur, const T& prev, float alpha)
{ return prev + alpha * (cur-prev); }

void FluidMechanics::Impl::setMatrices(const Matrix4& volumeMatrix, const Matrix4& stylusMatrix)
{
	synchronized(state->modelMatrix) {
		state->modelMatrix = volumeMatrix;
	}

	synchronized(state->stylusModelMatrix) {
		state->stylusModelMatrix = stylusMatrix;
	}

	updateSlicePlanes();
}

void FluidMechanics::Impl::updateSlicePlanes()
{
	if (state->stylusVisible) {
		if (state->tangibleVisible) { // <-- because of posToDataCoords()
			// Effector 2
			const float size = 0.5f * (stylusEffectorDist + std::max(dataSpacing.x*dataDim[0], std::max(dataSpacing.y*dataDim[1], dataSpacing.z*dataDim[2])));
			Vector3 dataPos = posToDataCoords(state->stylusModelMatrix * Matrix4::makeTransform(Vector3(-size, 0, 0)*settings->zoomFactor) * Vector3::zero());

			if (dataPos.x >= 0 && dataPos.y >= 0 && dataPos.z >= 0
			    && dataPos.x < dataDim[0]*dataSpacing.x && dataPos.y < dataDim[1]*dataSpacing.y && dataPos.z < dataDim[2]*dataSpacing.z)
			{
				// const auto rayPlaneIntersection2 = [](const Vector3& rayPoint, const Vector3& rayDir, const Vector3& planePoint, const Vector3& planeNormal, float& t) -> bool {
				// 	float dot = rayDir.dot(planeNormal);
				// 	if (dot != 0) {
				// 		t = -(rayPoint.dot(planeNormal) - planeNormal.dot(planePoint)) / dot;
				// 		LOGD("rayPlaneIntersection2 %s %s %s %s => %f", Utility::toString(rayPoint).c_str(), Utility::toString(rayDir).c_str(), Utility::toString(planePoint).c_str(), Utility::toString(planeNormal).c_str(), t);
				// 		return true;
				// 	} else {
				// 		LOGD("rayPlaneIntersection2 %s %s %s %s => [dot=%f]", Utility::toString(rayPoint).c_str(), Utility::toString(rayDir).c_str(), Utility::toString(planePoint).c_str(), Utility::toString(planeNormal).c_str(), dot);
				// 		return false;
				// 	}
				// };
				const auto rayAABBIntersection = [](const Vector3& rayPoint, const Vector3& rayDir, const Vector3& aabbMin, const Vector3& aabbMax, float& tmin, float& tmax) -> bool {
					// http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-box-intersection/
					float tmin_ = (aabbMin.x - rayPoint.x) / rayDir.x;
					float tmax_ = (aabbMax.x - rayPoint.x) / rayDir.x;
					if (tmin_ > tmax_) std::swap(tmin_, tmax_);
					float tymin = (aabbMin.y - rayPoint.y) / rayDir.y;
					float tymax = (aabbMax.y - rayPoint.y) / rayDir.y;
					if (tymin > tymax) std::swap(tymin, tymax);
					if ((tmin_ > tymax) || (tymin > tmax_))
						return false;
					if (tymin > tmin_)
						tmin_ = tymin;
					if (tymax < tmax_)
						tmax_ = tymax;
					float tzmin = (aabbMin.z - rayPoint.z) / rayDir.z;
					float tzmax = (aabbMax.z - rayPoint.z) / rayDir.z;
					if (tzmin > tzmax) std::swap(tzmin, tzmax);
					if ((tmin_ > tzmax) || (tzmin > tmax_))
						return false;
					if (tzmin > tmin_)
						tmin_ = tzmin;
					if (tzmax < tmax_)
						tmax_ = tzmax;
					if ((tmin_ > tmax) || (tmax_ < tmin)) return false;
					if (tmin < tmin_) tmin = tmin_;
					if (tmax > tmax_) tmax = tmax_;
					// LOGD("tmin = %f, tmax = %f", tmin, tmax);
					return true;
				};

				// const auto getAABBNormalAt = [](Vector3 point, const Vector3& aabbMin, const Vector3& aabbMax) -> Vector3 {
				// 	const auto sign = [](float value) {
				// 		return (value >= 0 ? 1 : -1);
				// 	};

				// 	// http://www.gamedev.net/topic/551816-finding-the-aabb-surface-normal-from-an-intersection-point-on-aabb/#entry4549909
				// 	Vector3 normal = Vector3::zero();
				// 	float min = std::numeric_limits<float>::max();
				// 	float distance;

				// 	Vector3 extents = aabbMax-aabbMin;
				// 	Vector3 center = (aabbMax+aabbMin)/2;

				// 	point -= center;

				// 	LOGD("point = %s, extents = %s", Utility::toString(point).c_str(), Utility::toString(extents).c_str());

				// 	distance = std::abs(extents.x - std::abs(point.x));
				// 	if (distance < min) {
				// 		min = distance;
				// 		normal = sign(point.x) * Vector3::unitX();
				// 	}

				// 	distance = std::abs(extents.y - std::abs(point.y));
				// 	if (distance < min) {
				// 		min = distance;
				// 		normal = sign(point.y) * Vector3::unitY();
				// 	}

				// 	distance = std::abs(extents.z - std::abs(point.z));
				// 	if (distance < min) {
				// 		min = distance;
				// 		normal = sign(point.z) * Vector3::unitZ();
				// 	}

				// 	return normal;
				// };

				// Same as posToDataCoords(), but for directions (not positions)
				// (direction goes from the effector to the stylus: +X axis)
				Vector3 dataDir = state->modelMatrix.transpose().get3x3Matrix() * state->stylusModelMatrix.inverse().transpose().get3x3Matrix() * Vector3::unitX();

				// static const float min = 0.0f;
				// const float max = settings->zoomFactor;
				// float t;
				float tmin = 0, tmax = 10000;
				synchronized (effectorIntersection) {
					// effectorIntersection = Vector3::zero();
					effectorIntersectionValid = false;
					// if (rayPlaneIntersection2(dataPos, dataDir, Vector3(0, 0, 0), -Vector3::unitX(), t) && t >= min && t <= max)
					// 	effectorIntersection = dataCoordsToPos(dataPos + dataDir*t);
					// if (rayPlaneIntersection2(dataPos, dataDir, Vector3(dataDim[0]*dataSpacing.x, 0, 0), Vector3::unitX(), t) && t >= min && t <= max)
					// 	effectorIntersection = dataCoordsToPos(dataPos + dataDir*t);
					// if (rayPlaneIntersection2(dataPos, dataDir, Vector3(0, 0, 0), -Vector3::unitY(), t) && t >= min && t <= max)
					// 	effectorIntersection = dataCoordsToPos(dataPos + dataDir*t);
					// if (rayPlaneIntersection2(dataPos, dataDir, Vector3(0, dataDim[1]*dataSpacing.y, 0), Vector3::unitY(), t) && t >= min && t <= max)
					// 	effectorIntersection = dataCoordsToPos(dataPos + dataDir*t);
					// if (rayPlaneIntersection2(dataPos, dataDir, Vector3(0, 0, 0), -Vector3::unitZ(), t) && t >= min && t <= max)
					// 	effectorIntersection = dataCoordsToPos(dataPos + dataDir*t);
					// if (rayPlaneIntersection2(dataPos, dataDir, Vector3(0, 0, dataDim[2]*dataSpacing.z), Vector3::unitZ(), t) && t >= min && t <= max)
					// 	effectorIntersection = dataCoordsToPos(dataPos + dataDir*t);
					if (rayAABBIntersection(dataPos, dataDir, Vector3::zero(), Vector3(dataDim[0], dataDim[1], dataDim[2])*dataSpacing, tmin, tmax) && tmax > 0) {
						effectorIntersection = dataCoordsToPos(dataPos + dataDir*tmax);
						// effectorIntersectionNormal = state->modelMatrix.transpose().get3x3Matrix() * getAABBNormalAt(dataPos + dataDir*tmax, Vector3::zero(), Vector3(dataDim[0], dataDim[1], dataDim[2])*dataSpacing);
						// LOGD("intersection = %s", Utility::toString(posToDataCoords(effectorIntersection)).c_str());
						// LOGD("intersection normal = %s", Utility::toString(getAABBNormalAt(dataPos + dataDir*tmax, Vector3::zero(), Vector3(dataDim[0], dataDim[1], dataDim[2])*dataSpacing)).c_str());
						effectorIntersectionValid = true;
					}
				}


				if (buttonIsPressed) {
					// settings->showSurface = true;
					settings->surfacePreview = true;

					vtkNew<vtkPoints> points;
					points->InsertNextPoint(dataPos.x, dataPos.y, dataPos.z);
					vtkNew<vtkPolyData> polyData;
					polyData->SetPoints(points.GetPointer());
					probeFilter->SetInputData(polyData.GetPointer());
					probeFilter->Update();

					vtkDataArray* scalars = probeFilter->GetOutput()->GetPointData()->GetScalars();
					android_assert(scalars);
					unsigned int num = scalars->GetNumberOfTuples();
					android_assert(num > 0);
					double value = scalars->GetComponent(0, 0);
					static double prevValue = 0.0;
					if (prevValue != 0.0)
						value = lowPassFilter(value, prevValue, 0.5f);
					prevValue = value;
					double range[2] = { volume->getMinValue(), volume->getMaxValue() };
					// LOGD("probed value = %f (range = %f / %f)", value, range[0], range[1]);
					settings->surfacePercentage = (value - range[0]) / (range[1] - range[0]);

					// NOTE: updateSurfacePreview cannot be used to update isosurfaceLow
					// from settings->surfacePercentage, since isosurfaceLow and isosurface
					// have different value ranges, hence expect different percentages.
					// updateSurfacePreview();

					// Directly use setValue() instead
					synchronized_if(isosurfaceLow) {
						isosurfaceLow->setValue(value);
					}
				}
			} else {
				effectorIntersectionValid = false;

				// // settings->showSurface = false;
				// settings->showSurface = true;
				// settings->surfacePreview = true;
			}
		}
	}

	bool clipPlaneSet = false;

	if (settings->showSlice && slice) {
		switch (settings->sliceType) {
			case SLICE_CAMERA:
				clipPlaneSet = computeCameraClipPlane(slicePoint, sliceNormal);
				break;

			case SLICE_AXIS:
				clipPlaneSet = computeAxisClipPlane(slicePoint, sliceNormal);
				break;

			case SLICE_STYLUS:
				clipPlaneSet = computeStylusClipPlane(slicePoint, sliceNormal);
				break;
		}
	}

	if (clipPlaneSet) {
		synchronized_if(isosurface) { isosurface->setClipPlane(sliceNormal.x, sliceNormal.y, sliceNormal.z, -sliceNormal.dot(slicePoint)); }
		synchronized_if(isosurfaceLow) { isosurfaceLow->setClipPlane(sliceNormal.x, sliceNormal.y, sliceNormal.z, -sliceNormal.dot(slicePoint)); }
		synchronized_if(volume) { volume->setClipPlane(sliceNormal.x, sliceNormal.y, sliceNormal.z, -sliceNormal.dot(slicePoint)); }

		// pt: data space
		// dir: eye space
		const auto rayPlaneIntersection = [this](const Vector3& pt, const Vector3& dir, float& t) -> bool {
			// float dot = dir.dot(posToDataCoords(sliceNormal));
			// float dot = dataCoordsToPos(dir).dot(sliceNormal);
			float dot = dir.dot(sliceNormal);
			if (dot == 0)
				return false;
			// t = -(pt.dot(posToDataCoords(sliceNormal)) - sliceNormal.dot(slicePoint)) / dot;
			t = -(dataCoordsToPos(pt).dot(sliceNormal) - sliceNormal.dot(slicePoint)) / dot;
			// t = -(pt.dot(sliceNormal) - sliceNormal.dot(slicePoint)) / dot;
			// LOGD("t = %f", t);
			return true;
		};

		// Slice-cube intersection
		float t;
		Vector3 dir;
		synchronized(slicePoints) {
			slicePoints.clear();

			// dir = Vector3(dataDim[0]*dataSpacing.x, 0, 0);
			static const float min = 0.0f;
			// static const float max = 1.0f;
			// static const float max = 1.15f; // FIXME: why?
			// static const float max = settings->zoomFactor;
			// static const float max = 4.6f; // 10.5 (ironProt), 1.15 (head)
			// // D/NativeApp(24843): 2.135922 1.067961 103 1.000000
			// // D/NativeApp(24843): 3.235294 1.617647 68 1.000000
			// // D/NativeApp(24843): 1.074219 0.537109 64 3.200000
			const float max = settings->zoomFactor;// * settings->zoomFactor;

			// static const float max = 2*settings->zoomFactor*state->computedZoomFactor;
			// LOGD("%f %f %d %f", settings->zoomFactor, state->computedZoomFactor, dataDim[0], dataSpacing.x);

			// Same as dataCoordsToPos(), but for directions (not positions)
			dir = state->modelMatrix.inverse().transpose().get3x3Matrix() * (Vector3(dataDim[0]*dataSpacing.x, 0, 0));// / settings->zoomFactor);

			if (rayPlaneIntersection(Vector3(0, 0, 0), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(0, 0, 0)) + dir*t);
			if (rayPlaneIntersection(Vector3(0, dataDim[1]*dataSpacing.y, 0), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(0, dataDim[1]*dataSpacing.y, 0)) + dir*t);
			if (rayPlaneIntersection(Vector3(0, 0, dataDim[2]*dataSpacing.z), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(0, 0, dataDim[2]*dataSpacing.z)) + dir*t);
			if (rayPlaneIntersection(Vector3(0, dataDim[1]*dataSpacing.y, dataDim[2]*dataSpacing.z), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(0, dataDim[1]*dataSpacing.y, dataDim[2]*dataSpacing.z)) + dir*t);

			// dir = Vector3(0, dataDim[1]*dataSpacing.y, 0);
			dir = state->modelMatrix.inverse().transpose().get3x3Matrix() * (Vector3(0, dataDim[1]*dataSpacing.y, 0));// / settings->zoomFactor);
			if (rayPlaneIntersection(Vector3(0, 0, 0), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(0, 0, 0)) + dir*t);
			if (rayPlaneIntersection(Vector3(dataDim[0]*dataSpacing.x, 0, 0), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(dataDim[0]*dataSpacing.x, 0, 0)) + dir*t);
			if (rayPlaneIntersection(Vector3(0, 0, dataDim[2]*dataSpacing.z), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(0, 0, dataDim[2]*dataSpacing.z)) + dir*t);
			if (rayPlaneIntersection(Vector3(dataDim[0]*dataSpacing.x, 0, dataDim[2]*dataSpacing.z), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(dataDim[0]*dataSpacing.x, 0, dataDim[2]*dataSpacing.z)) + dir*t);

			// dir = Vector3(0, 0, dataDim[2]*dataSpacing.z);
			dir = state->modelMatrix.inverse().transpose().get3x3Matrix() * (Vector3(0, 0, dataDim[2]*dataSpacing.z));// / settings->zoomFactor);
			if (rayPlaneIntersection(Vector3(0, 0, 0), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(0, 0, 0)) + dir*t);
			if (rayPlaneIntersection(Vector3(dataDim[0]*dataSpacing.x, 0, 0), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(dataDim[0]*dataSpacing.x, 0, 0)) + dir*t);
			if (rayPlaneIntersection(Vector3(0, dataDim[1]*dataSpacing.y, 0), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(0, dataDim[1]*dataSpacing.y, 0)) + dir*t);
			if (rayPlaneIntersection(Vector3(dataDim[0]*dataSpacing.x, dataDim[1]*dataSpacing.y, 0), dir, t) && t >= min && t <= max)
				slicePoints.push_back(dataCoordsToPos(Vector3(dataDim[0]*dataSpacing.x, dataDim[1]*dataSpacing.y, 0)) + dir*t);

			// LOGD("slicePoints.size() = %d", slicePoints.size());
		}
	} else {
		synchronized_if(isosurface) { isosurface->clearClipPlane(); }
		synchronized_if(isosurfaceLow) { isosurfaceLow->clearClipPlane(); }
		synchronized_if(volume) { volume->clearClipPlane(); }
	}
}

// (GL context)
void FluidMechanics::Impl::renderObjects()
{
	const Matrix4 proj = app->getProjMatrix();

	glEnable(GL_DEPTH_TEST);

	// XXX: test
	Matrix4 mm;
	synchronized(state->modelMatrix) {
		mm = state->modelMatrix;
	}
	glDisable(GL_BLEND);
	synchronized_if(isosurface) {
		glDepthMask(true);
		glDisable(GL_CULL_FACE);
		isosurface->render(proj, mm);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDepthMask(true); // requires "discard" in the shader where alpha == 0

	if (settings->clipDist > 0.0f) {
		// Set a depth value for the slicing plane
		Matrix4 trans = Matrix4::identity();
		// trans[3][2] = app->getDepthValue(settings->clipDist); // relative to trans[3][3], which is 1.0
		trans[3][2] = app->getDepthValue(sliceDepth);
		// LOGD("%s", Utility::toString(trans).c_str());

		trans[1][1] *= -1; // flip the texture vertically, because of "orthoProjMatrix"

		synchronized(slice) {
			slice->setOpaque(false);
			slice->render(app->getOrthoProjMatrix(), trans);
		}
	}

	synchronized(slicePoints) {
		if (!slicePoints.empty()) {
			std::vector<Vector3> lineVec;
			std::map<unsigned int, std::map<unsigned int, float>> graph;
			for (unsigned int i = 0; i < slicePoints.size(); ++i) {
				for (unsigned int j = 0; j < slicePoints.size(); ++j) {
					// std::pair<unsigned int, unsigned int> pair(i, j);
					// if (i == j || pairs.count(pair))
					if (i == j || (graph.count(j) && graph.at(j).count(i)))
						continue;
					const Vector3 pt1 = slicePoints.at(i);
					const Vector3 pt2 = slicePoints.at(j);
					const Vector3 dpt1 = posToDataCoords(pt1);
					const Vector3 dpt2 = posToDataCoords(pt2);
					static const float epsilon = 0.1f;
					if (std::abs(dpt1.x-dpt2.x) < epsilon || std::abs(dpt1.y-dpt2.y) < epsilon || std::abs(dpt1.z-dpt2.z) < epsilon) {
						// float dot = (pt2 - pt1).normalized().dot((center - pt1).normalized());
						// LOGD("dot = %f", dot);
						// if (dot < 0.9f) {
						lineVec.push_back(pt1);
						lineVec.push_back(pt2);
					}
					// pairs.insert(pair);
					// graph[i][j] = pt1.distance(pt2);
					// }
				}
			}
			lines->setLines(lineVec);
			// glLineWidth(1.0f);
			glLineWidth(5.0f);
			lines->setColor(Vector3(0, 1, 0));
			glDisable(GL_DEPTH_TEST);
			lines->render(proj, Matrix4::identity());
		}
	}

	glEnable(GL_DEPTH_TEST);
	synchronized_if(volume) {
		// glDepthMask(false);
		glDepthMask(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // modulate
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
		glDisable(GL_CULL_FACE);
		volume->render(proj, mm);
	}
	synchronized_if(outline) {
		glDepthMask(true);
		glLineWidth(2.0f);
		outline->setColor(Vector3(1.0f, 0, 0));
		outline->render(proj, mm*Matrix4::makeTransform(
			                Vector3::zero(),
			                Quaternion::identity(),
			                Vector3(1.01f)));
	}


	return;

	// Stylus (paddle?) z-buffer occlusion
	// TODO: correct occlusion shape for the real *stylus*
	if (state->stylusVisible && cube /*&& (settings->sliceType != SLICE_STYLUS || slice->isEmpty())*/) {
		glColorMask(false, false, false, false);
		glDepthMask(true);

		// if (qcarVisible) {
		// 	static const Matrix4 transform = Matrix4::makeTransform(
		// 		Vector3::zero(),
		// 		Quaternion::identity(),
		// 		Vector3(63, 63, 3)/2
		// 	);
		//
		// 	synchronized (state->stylusModelMatrix) {
		// 		cube->render(proj, state->stylusModelMatrix*transform);
		// 	}
		// } else {
		// 	bool isRfduinoStylus = false;
		// 	try {
		// 		std::string curMarkerFileName = stylus.getMarkers().at(stylus.getCurrentMarkerID()).patternFileName;
		// 		isRfduinoStylus = (curMarkerFileName == "14.patt" || curMarkerFileName == "24.patt" || curMarkerFileName == "26.patt");
		// 	} catch (...) {
		// 		// ...
		// 	}

			synchronized (state->stylusModelMatrix) {
				// if (!isRfduinoStylus) {
					cube->render(proj, state->stylusModelMatrix
					             * Matrix4::makeTransform(
						             Vector3(10.0, 0, 10.0),
						             Quaternion::identity(),
						             Vector3(59, 40, 3)/2
					             ));
					cube->render(proj, state->stylusModelMatrix
					             * Matrix4::makeTransform(
						             Vector3(10.0, -10.0, -5.0),
						             // Quaternion(Vector3::unitX(),  2.146),
						             Quaternion(Vector3::unitX(),  2.09),
						             Vector3(59, 40, 3)/2
					             ));
					cube->render(proj, state->stylusModelMatrix
					             * Matrix4::makeTransform(
						             Vector3(10.0, 10.0, -5.0),
						             // Quaternion(Vector3::unitX(), -2.146),
						             Quaternion(Vector3::unitX(), -2.09),
						             Vector3(59, 40, 3)/2
					             ));
					// Handle
					if (cylinder) {
						cylinder->render(proj, state->stylusModelMatrix
						                 * Matrix4::makeTransform(
							                 Vector3(75.0, 0.0, 0.0),
							                 Quaternion(Vector3::unitY(), M_PI/2),
							                 Vector3(0.01f, 0.01f, 0.017f)*2
						                 ));
					}
				// } else {
				// 	cube->render(proj, state->stylusModelMatrix
				// 	             * Matrix4::makeTransform(
				// 		             Vector3(11.0, 0, 18.0),
				// 		             Quaternion::identity(),
				// 		             Vector3(57, 40, 3)/2
				// 	             ));
				// 	cube->render(proj, state->stylusModelMatrix
				// 	             * Matrix4::makeTransform(
				// 		             Vector3(11.0, 0, -18.0),
				// 		             Quaternion::identity(),
				// 		             Vector3(57, 40, 3)/2
				// 	             ));
				// 	cube->render(proj, state->stylusModelMatrix
				// 	             * Matrix4::makeTransform(
				// 		             Vector3(11.0, 18.0, 0.0),
				// 		             Quaternion(Vector3::unitX(), -M_PI/2),
				// 		             Vector3(57, 40, 3)/2
				// 	             ));
				// 	cube->render(proj, state->stylusModelMatrix
				// 	             * Matrix4::makeTransform(
				// 		             Vector3(11.0, -18.0, 0.0),
				// 		             Quaternion(Vector3::unitX(), M_PI/2),
				// 		             Vector3(57, 40, 3)/2
				// 	             ));
				// }
			}
		// }

		glColorMask(true, true, true, true);
	}

	if (settings->showStylus && state->stylusVisible && cube) {
		glDepthMask(true);
		glEnable(GL_CULL_FACE);

		Matrix4 smm;
		synchronized(state->stylusModelMatrix) {
			smm = state->stylusModelMatrix;
		}

#ifndef NEW_STYLUS_RENDER
		// Effector
		static const Matrix4 transform1 = Matrix4::makeTransform(
			Vector3(0, 0, 0), // (after scaling)
			Quaternion::identity(),
			Vector3(10.0f)
		);
		cube->setColor(Vector3(1.0f));
		// LOGD("render 1");
		cube->render(proj, smm*transform1);
		// LOGD("render 1 success");
		// cube->render(qcarProjMatrix, mm*transform1);

		// Handle
		static const Matrix4 transform2 = Matrix4::makeTransform(
			// Vector3((130/*+105*/)*0.5f, 0, 0), // (after scaling)
			Vector3((130+25)*0.5f, 0, 0), // (after scaling)
			Quaternion::identity(),
			// Vector3((130/*+105*/)*0.5f, 5.0f, 5.0f)
			Vector3((130+25)*0.5f, 5.0f, 5.0f)
		);
		cube->setColor(Vector3(0.7f));
		// LOGD("render 2");
		cube->render(proj, smm*transform2);
		// LOGD("render 2 success");
		// cube->render(qcarProjMatrix, mm*transform2);

		if (state->tangibleVisible) { // <-- because of posToDataCoords()
			// Effector 2
			const float size = 0.5f * (stylusEffectorDist + std::max(dataSpacing.x*dataDim[0], std::max(dataSpacing.y*dataDim[1], dataSpacing.z*dataDim[2])));
			Vector3 dataPos = posToDataCoords(smm * Matrix4::makeTransform(Vector3(-size, 0, 0)*settings->zoomFactor) * Vector3::zero());
			if (dataPos.x >= 0 && dataPos.y >= 0 && dataPos.z >= 0
			    && dataPos.x < dataDim[0] && dataPos.y < dataDim[1] && dataPos.z < dataDim[2])
			{
				cube->setColor(Vector3(0.5f));
				// cube->setOpacity(1.0f);
			} else {
				cube->setColor(Vector3(1, 0.5, 0.5));
				// cube->setOpacity(0.5f);
			}

			const Matrix4 transform3 = Matrix4::makeTransform(
				Vector3(-size, 0, 0) * settings->zoomFactor,
				Quaternion::identity(),
				Vector3(2.0f * settings->zoomFactor)
				// Vector3(0.3f * settings->zoomFactor)
			);

			// LOGD("render 3");
			cube->render(proj, smm*transform3);
			// cube->render(qcarProjMatrix, mm*transform3);
			// particleSphere->render(proj, mm*transform3);
			// LOGD("render 3 success");
		}
#else
		if (settings->sliceType == SLICE_STYLUS) {
			// // Effector
			// static const Matrix4 transform1 = Matrix4::makeTransform(
			// 	Vector3(0, 0, 0), // (after scaling)
			// 	Quaternion::identity(),
			// 	Vector3(10.0f)
			// );
			// cube->setColor(Vector3(1.0f));
			// // LOGD("render 1");
			// cube->render(proj, smm*transform1);
			// // LOGD("render 1 success");
			// // cube->render(qcarProjMatrix, smm*transform1);
			//
			// // Handle
			// static const Matrix4 transform2 = Matrix4::makeTransform(
			// 	// Vector3((130/*+105*/)*0.5f, 0, 0), // (after scaling)
			// 	Vector3((130+25)*0.5f, 0, 0), // (after scaling)
			// 	Quaternion::identity(),
			// 	// Vector3((130/*+105*/)*0.5f, 5.0f, 5.0f)
			// 	Vector3((130+25)*0.5f, 5.0f, 5.0f)
			// );
			// cube->setColor(Vector3(0.7f));
			// // LOGD("render 2");
			// cube->render(proj, smm*transform2);
			// // LOGD("render 2 success");
			// // cube->render(qcarProjMatrix, smm*transform2);

		} else {
		const float size = 0.5f * (stylusEffectorDist + std::max(dataSpacing.x*dataDim[0], std::max(dataSpacing.y*dataDim[1], dataSpacing.z*dataDim[2])));

		// Handle
		const Matrix4 transform2 = Matrix4::makeTransform(
			// Vector3((130/*+105*/)*0.5f, 0, 0), // (after scaling)
			Vector3(-size*0.5*settings->zoomFactor, 0, 0), // (after scaling)
			Quaternion::identity(),
			// Vector3((130/*+105*/)*0.5f, 5.0f, 5.0f)
			Vector3(size*0.5*settings->zoomFactor, 2.0f, 2.0f)
		);

		if (!state->tangibleVisible) {
		// if (!state->tangibleVisible || settings->sliceType == SLICE_STYLUS) {
			// Handle
			cube->setColor(Vector3(0.7f));
			cube->render(proj, smm*transform2);

		// if (state->tangibleVisible) { // <-- because of posToDataCoords()
		} else { // <-- because of posToDataCoords()
			Vector3 effectorPos = smm * Matrix4::makeTransform(Vector3(-size, 0, 0)*settings->zoomFactor) * Vector3::zero();
			Vector3 dataPos = posToDataCoords(effectorPos);
			const bool insideVolume = (dataPos.x >= 0 && dataPos.y >= 0 && dataPos.z >= 0
				&& dataPos.x < dataDim[0]*dataSpacing.x && dataPos.y < dataDim[1]*dataSpacing.y && dataPos.z < dataDim[2]*dataSpacing.z);
			if (insideVolume) {
				// cube->setColor(Vector3(0.5f));
				// cube->setColor(!mousePressed ? Vector3(0.5f) : Vector3(0.5f, 1.0f, 0.5f));
				cube->setColor(Vector3(0.5f));
				// cube->setOpacity(1.0f);
			} else {
				cube->setColor(Vector3(1, 0.5, 0.5));
				// cube->setOpacity(0.5f);
			}

			// Handle
			cube->render(proj, smm*transform2);

			// Effector 2
			const Matrix4 transform3 = Matrix4::makeTransform(
				Vector3(-size, 0, 0) * settings->zoomFactor,
				Quaternion::identity(),
				// Vector3(2.0f * settings->zoomFactor)
				Vector3(2.5f * settings->zoomFactor)
				// Vector3(0.3f * settings->zoomFactor)
			);

			// LOGD("render 3");
			cube->render(proj, smm*transform3);
			// cube->render(qcarProjMatrix, smm*transform3);
			// particleSphere->render(proj, smm*transform3);
			// LOGD("render 3 success");

			cube->setColor(Vector3(0.5f));

			if (insideVolume && settings->showCrossingLines) {
				// Show crossing axes to help the user locate
				// the effector position in the data
				Matrix4 mm;
				synchronized(state->modelMatrix) {
					mm = state->modelMatrix;
				}

				glLineWidth(2.0f);
				axisCube->setColor(Vector3(1.0f));
				// axisCube->setColor(mousePressed ? Vector3(1.0f) : Vector3(0.7f));

				axisCube->render(proj, mm*Matrix4::makeTransform(mm.inverse()*effectorPos*Vector3(0,1,1), Quaternion::identity(), Vector3(0.5*dataDim[0]*dataSpacing.x*settings->zoomFactor, 0, 0)));
				cube->render(proj, mm*Matrix4::makeTransform(mm.inverse()*effectorPos*Vector3(0,1,1)-Vector3(0.5*dataDim[0]*dataSpacing.x*settings->zoomFactor,0,0), Quaternion::identity(), Vector3(0.25, 2, 2)));
				cube->render(proj, mm*Matrix4::makeTransform(mm.inverse()*effectorPos*Vector3(0,1,1)+Vector3(0.5*dataDim[0]*dataSpacing.x*settings->zoomFactor,0,0), Quaternion::identity(), Vector3(0.25, 2, 2)));

				axisCube->render(proj, mm*Matrix4::makeTransform(mm.inverse()*effectorPos*Vector3(1,0,1), Quaternion::identity(), Vector3(0, 0.5*dataDim[1]*dataSpacing.y*settings->zoomFactor, 0)));
				cube->render(proj, mm*Matrix4::makeTransform(mm.inverse()*effectorPos*Vector3(1,0,1)-Vector3(0,0.5*dataDim[1]*dataSpacing.y*settings->zoomFactor,0), Quaternion::identity(), Vector3(2, 0.25, 2)));
				cube->render(proj, mm*Matrix4::makeTransform(mm.inverse()*effectorPos*Vector3(1,0,1)+Vector3(0,0.5*dataDim[1]*dataSpacing.y*settings->zoomFactor,0), Quaternion::identity(), Vector3(2, 0.25, 2)));

				axisCube->render(proj, mm*Matrix4::makeTransform(mm.inverse()*effectorPos*Vector3(1,1,0), Quaternion::identity(), Vector3(0, 0, 0.5*dataDim[2]*dataSpacing.z*settings->zoomFactor)));
				cube->render(proj, mm*Matrix4::makeTransform(mm.inverse()*effectorPos*Vector3(1,1,0)-Vector3(0,0,0.5*dataDim[2]*dataSpacing.y*settings->zoomFactor), Quaternion::identity(), Vector3(2, 2, 0.25)));
				cube->render(proj, mm*Matrix4::makeTransform(mm.inverse()*effectorPos*Vector3(1,1,0)+Vector3(0,0,0.5*dataDim[2]*dataSpacing.y*settings->zoomFactor), Quaternion::identity(), Vector3(2, 2, 0.25)));
			}

#if 0
			synchronized (effectorIntersection) {
				if (effectorIntersectionValid) {
					// Effector intersection
					const Matrix4 transform4 = Matrix4::makeTransform(
						effectorIntersection,
						Quaternion::identity(),
						// Vector3::unitZ().rotationTo(effectorIntersectionNormal),
						Vector3(3.0f * settings->zoomFactor)
					);
					// cube->render(proj, smm*transform4);
					cube->render(proj, transform4);
				}
			}
#endif
		}
		}
#endif
	}

	// LOGD("render 4");

	if (state->tangibleVisible) {
		Matrix4 mm;
		synchronized(state->modelMatrix) {
			mm = state->modelMatrix;
		}

		// Apply the zoom factor
		mm = mm * Matrix4::makeTransform(
			Vector3::zero(),
			Quaternion::identity(),
			Vector3(settings->zoomFactor)
		);

		synchronized_if(outline) {
			glDepthMask(true);
			glLineWidth(2.0f);
			outline->setColor(!velocityData ? Vector3(1.0f, 0, 0) : Vector3(0, 1.0f, 0));
			outline->render(proj, mm);
		}

		// Render the surface
		if (settings->showSurface) {
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDepthMask(true);

			if (!settings->surfacePreview || !isosurfaceLow) {
				synchronized_if(isosurface) {
					isosurface->render(proj, mm);
				}
			}

			if (settings->surfacePreview) {
				synchronized_if(isosurfaceLow) {
					isosurfaceLow->render(proj, mm);
				}
			}
		}

		// Render particles
		synchronized (particles) {
			for (Particle& p : particles) {
				if (!p.valid)
					continue;
				integrateParticleMotion(p);
				if (!p.valid || p.delayMs > 0)
					continue;
				Vector3 pos = p.pos;
				pos -= Vector3(dataDim[0]/2, dataDim[1]/2, dataDim[2]/2) * dataSpacing;
				// particleSphere->render(proj, mm * Matrix4::makeTransform(pos, Quaternion::identity(), Vector3(0.3f)));
				// particleSphere->render(proj, mm * Matrix4::makeTransform(pos, Quaternion::identity(), Vector3(0.2f)));
				particleSphere->render(proj, mm * Matrix4::makeTransform(pos, Quaternion::identity(), Vector3(0.15f)));
			}
		}

		// NOTE: must be rendered before "slice" (because of
		// transparency sorting)
		if (settings->showSlice && state->clipAxis != CLIP_NONE && state->lockedClipAxis == CLIP_NONE) {
			Vector3 scale;
			Vector3 color;
			switch (state->clipAxis) {
				case CLIP_AXIS_X: case CLIP_NEG_AXIS_X: scale = Vector3(150, 0, 0); color = Vector3(1, 0, 0); break;
				case CLIP_AXIS_Y: case CLIP_NEG_AXIS_Y: scale = Vector3(0, 150, 0); color = Vector3(0, 1, 0); break;
				case CLIP_AXIS_Z: case CLIP_NEG_AXIS_Z: scale = Vector3(0, 0, 150); color = Vector3(0, 0, 1); break;
				case CLIP_NONE: android_assert(false);
			}

			const Matrix4 trans = Matrix4::makeTransform(
				Vector3::zero(),
				Quaternion::identity(),
				scale
			);

			glDepthMask(true);
			glLineWidth(5.0f);
			axisCube->setColor(color);
			axisCube->render(proj, mm*trans);
		}

		// FIXME: slight misalignment error?
		// const float sliceDot = (settings->showSlice ? sliceNormal.dot(Vector3::unitZ()) : 0);

		// Render the volume
		if (settings->showVolume) {// && sliceDot <= 0) {
			synchronized_if(volume) {
				glDepthMask(false);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // modulate
				// glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
				glDisable(GL_CULL_FACE);
				volume->render(proj, mm);
			}
		}

		if (slice && settings->showSlice) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_CULL_FACE);
			glDepthMask(true); // requires "discard" in the shader where alpha == 0

			switch (settings->sliceType) {
				case SLICE_CAMERA: {
					if (settings->clipDist > 0.0f) {
						// Set a depth value for the slicing plane
						Matrix4 trans = Matrix4::identity();
						// trans[3][2] = app->getDepthValue(settings->clipDist); // relative to trans[3][3], which is 1.0
						trans[3][2] = app->getDepthValue(sliceDepth);
						// LOGD("%s", Utility::toString(trans).c_str());

						trans[1][1] *= -1; // flip the texture vertically, because of "orthoProjMatrix"

						synchronized(slice) {
							slice->setOpaque(false);
							slice->render(app->getOrthoProjMatrix(), trans);
						}
					}

					break;
				}

				case SLICE_AXIS:
				case SLICE_STYLUS: {
					if (settings->sliceType != SLICE_STYLUS || state->stylusVisible) {
						Matrix4 s2mm;
						synchronized(state->sliceModelMatrix) {
							s2mm = state->sliceModelMatrix;
						}

						synchronized(slice) {
							slice->setOpaque(settings->sliceType == SLICE_STYLUS || slice->isEmpty());
							slice->render(proj, s2mm);
						}

#if 0
						synchronized(slicePoints) {
							// for (const Vector3& pt : slicePoints) {
							// 	// LOGD("pt = %s", Utility::toString(pt).c_str());
							// 	const Matrix4 transform1 = Matrix4::makeTransform(
							// 		// Vector3(0, 0, 0), // (after scaling)
							// 		pt,
							// 		Quaternion::identity(),
							// 		Vector3(10.0f)
							// 	);
							// 	cube->setColor(Vector3(1.0f));
							// 	cube->setOpacity(1.0f);
							// 	// cube->render(proj, Matrix4::makeTransform(pt)*transform1);
							// 	cube->render(proj, transform1);
							// }
							// // LOGD("============");
							if (!slicePoints.empty()) {
								// Vector3 center = Vector3::zero();
								// for (const Vector3& pt : slicePoints) {
								// 	center += pt;
								// }
								// center /= slicePoints.size();

								// std::vector<Vector3> lineVec;
								// std::map<unsigned int, std::map<unsigned int, float>> graph;
								// for (unsigned int i = 0; i < slicePoints.size(); ++i) {
								// 	for (unsigned int j = 0; j < slicePoints.size(); ++j) {
								// 		if (i == j) // || (graph.count(j) && graph.at(j).count(i)))
								// 			continue;
								// 		const Vector3 pt1 = slicePoints.at(i);
								// 		const Vector3 pt2 = slicePoints.at(j);
								// 		graph[i][j] = (pt2 - pt1).normalized().dot((center - pt1).normalized());
								// 	}
								// }

								// for (const auto& pair : graph) {
								// 	typedef std::pair<unsigned int, float> PairT;
								// 	std::vector<PairT> dots;
								// 	for (const auto& pair2 : pair.second) {
								// 		dots.push_back(PairT(pair2.first, pair2.second));
								// 	}
								// 	// Get the two edges with the lowest dot products relative to "center"
								// 	std::sort(dots.begin(), dots.end(), [](const PairT& a, const PairT& b) { return a.second < b.second; });
								// 	dots.resize(2);
								// 	for (const auto& pair3 : dots) {
								// 		lineVec.push_back(slicePoints.at(pair.first));
								// 		lineVec.push_back(slicePoints.at(pair3.first));
								// 	}
								// }



								std::vector<Vector3> lineVec;
								// std::set<std::pair<unsigned int, unsigned int>> pairs;
								// struct LineStruct { Vector3 p1, p2; float dist; };
								// std::map<std::pair<unsigned int, unsigned int>, LineStruct> pairs;
								std::map<unsigned int, std::map<unsigned int, float>> graph;
								for (unsigned int i = 0; i < slicePoints.size(); ++i) {
									for (unsigned int j = 0; j < slicePoints.size(); ++j) {
										// std::pair<unsigned int, unsigned int> pair(i, j);
										// if (i == j || pairs.count(pair))
										if (i == j || (graph.count(j) && graph.at(j).count(i)))
											continue;
										const Vector3 pt1 = slicePoints.at(i);
										const Vector3 pt2 = slicePoints.at(j);
										const Vector3 dpt1 = posToDataCoords(pt1);
										const Vector3 dpt2 = posToDataCoords(pt2);
										static const float epsilon = 0.1f;
										if (std::abs(dpt1.x-dpt2.x) < epsilon || std::abs(dpt1.y-dpt2.y) < epsilon || std::abs(dpt1.z-dpt2.z) < epsilon) {
										// float dot = (pt2 - pt1).normalized().dot((center - pt1).normalized());
										// LOGD("dot = %f", dot);
										// if (dot < 0.9f) {
											lineVec.push_back(pt1);
											lineVec.push_back(pt2);
										}
											// pairs.insert(pair);
											// graph[i][j] = pt1.distance(pt2);
										// }
									}
								}
								// // for (auto& pair : graph) {
								// // 	// if (pair.second.size() <= 2) {
								// // 	// 	for (auto& pair3 : pair.second) {
								// // 	// 		lineVec.push_back(slicePoints.at(pair.first));
								// // 	// 		lineVec.push_back(slicePoints.at(pair3.first));
								// // 	// 	}
								// // 	// 	continue;
								// // 	// }
								// // 	typedef std::pair<unsigned int, float> PairT;
								// // 	std::vector<PairT> dists;
								// // 	for (auto& pair2 : pair.second) {
								// // 		dists.push_back(PairT(pair2.first, pair2.second));
								// // 	}
								// // 	// std::sort(dists.begin(), dists.end(), std::lower<float>());
								// // 	std::sort(dists.begin(), dists.end(), [](const PairT& a, const PairT& b) { return a.second < b.second; });
								// // 	dists.resize(2);
								// // 	for (auto& pair3 : dists) {
								// // 		lineVec.push_back(slicePoints.at(pair.first));
								// // 		lineVec.push_back(slicePoints.at(pair3.first));
								// // 	}
								// // }
								lines->setLines(lineVec);
								// glLineWidth(1.0f);
								glLineWidth(2.0f);
								lines->setColor(Vector3(0, 1, 0));
								lines->render(proj, Matrix4::identity());
							}
						}
#endif
					}

					break;
				}
			}
		}

		// // Render the volume after the slicing plane when the plane
		// // normal is facing the screen
		// if (settings->showVolume && sliceDot > 0) {
		// 	synchronized_if(volume) {
		// 		glDepthMask(false);
		// 		glEnable(GL_BLEND);
		// 		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // modulate
		// 		// glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
		// 		glDisable(GL_CULL_FACE);
		// 		volume->render(proj, mm);
		// 	}
		// }
	}

	// // XXX: debug
	// if (!settings->showVolume && !settings->showSurface) {
	// 	glEnable(GL_CULL_FACE);
	// 	synchronized(tangible) {
	// 		for (const auto& pair : tangible.getMarkers()) {
	// 			if (!pair.second.isVisible())
	// 				continue;
	//
	// 			// LOGD("marker %2d (main=%d) err=%f cf=%f", pair.first, pair.second.isMain, pair.second.err, pair.second.cf);
	//
	// 			// Center the cube on its native, and scale it up
	// 			const Matrix4 transform = Matrix4::makeTransform(
	// 				Vector3(0, 0, 0.5f), // (after scaling)
	// 				Quaternion::identity(),
	// 				Vector3(20.0f, 20.0f, 1.0f) * (pair.second.width/51.0)
	// 			);
	//
	// 			if (pair.first == tangible.getCurrentMarkerID())
	// 				cube->setColor(Vector3(0.25f, 1.0f, 0.25f));
	// 			else if (pair.second.dubious)
	// 				cube->setColor(Vector3(1.0f, 0.25f, 0.25f));
	// 			else
	// 				cube->setColor(Vector3(0.25f, 0.25f, 1.0f));
	//
	// 			cube->render(proj, pair.second.transform*transform);
	//
	// 			// LOGD("modelMatrix for marker %d = %s", pair.first, Utility::toString(pair.second.transform).c_str());
	// 		}
	// 		// LOGD("==========");
	// 	}
	// }

	// if (qcarVisible) {
	// 	synchronized (qcarModelMatrix) {
	// 		const Matrix4 transform = Matrix4::makeTransform(
	// 			Vector3(0, 0, 0.5f), // (after scaling)
	// 			Quaternion::identity(),
	// 			Vector3(30.0f, 30.0f, 30.0f)
	// 		);

	// 		cube->render(proj, qcarModelMatrix*transform);
	// 	}
	// }
}

void FluidMechanics::Impl::updateSurfacePreview()
{
	if (!settings->surfacePreview) {
		synchronized_if(isosurface) {
			isosurface->setPercentage(settings->surfacePercentage);
		}
	} else if (settings->surfacePreview) {
		synchronized_if(isosurfaceLow) {
			isosurfaceLow->setPercentage(settings->surfacePercentage);
		}
	}
}

FluidMechanics::FluidMechanics(const std::string& baseDir)
 : impl(new Impl(baseDir)),
   settings(new FluidMechanics::Settings),
   state(new FluidMechanics::State)
{
	impl->app = this;
	impl->settings = std::static_pointer_cast<FluidMechanics::Settings>(settings);
	impl->state = std::static_pointer_cast<FluidMechanics::State>(state);

	// Create a perspective projection matrix
	projMatrix = Matrix4::perspective(35.0f, float(SCREEN_WIDTH)/SCREEN_HEIGHT, 50.0f, 2500.0f);
	projMatrix[1][1] *= -1;
	projMatrix[2][2] *= -1;
	projMatrix[2][3] *= -1;

	projNearClipDist = -projMatrix[3][2] / (1+projMatrix[2][2]); // 50.0f
	projFarClipDist  =  projMatrix[3][2] / (1-projMatrix[2][2]); // 2500.0f

	// Create an orthographic projection matrix
	orthoProjMatrix = Matrix4::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f);
}

FluidMechanics::~FluidMechanics()
{
	//
}

bool FluidMechanics::loadDataSet(const std::string& fileName)
{
	return impl->loadDataSet(fileName);
}

bool FluidMechanics::loadVelocityDataSet(const std::string& fileName)
{
	return impl->loadVelocityDataSet(fileName);
}

void FluidMechanics::releaseParticles()
{
	impl->releaseParticles();
}

void FluidMechanics::buttonPressed()
{
	impl->buttonPressed();
}

float FluidMechanics::buttonReleased()
{
	return impl->buttonReleased();
}

void FluidMechanics::rebind()
{
	impl->rebind();
}

void FluidMechanics::setMatrices(const Matrix4& volumeMatrix, const Matrix4& stylusMatrix)
{
	impl->setMatrices(volumeMatrix, stylusMatrix);
}

void FluidMechanics::render()
{
	impl->renderObjects();
}

void FluidMechanics::updateSurfacePreview()
{
	impl->updateSurfacePreview();
}
