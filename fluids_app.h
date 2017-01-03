#ifndef FLUID_MECHANICS_H
#define FLUID_MECHANICS_H

#include "global.h"
#include "FillVolume.h"

class FluidMechanics
{
public:
	FluidMechanics(const std::string& baseDir);
	~FluidMechanics();

	// (GL context)
	void rebind();

	// (GL context)
	void render();

	void setMatrices(const Matrix4& volumeMatrix, const Matrix4& stylusMatrix);

	bool loadDataSet(const std::string& fileName);
	bool loadVelocityDataSet(const std::string& fileName);

	void updateSurfacePreview(); // must be called after updating the isosurface value in Settings

	void releaseParticles();
	void resetParticles();

	void buttonPressed();
	float buttonReleased();

	void setSeedPoint(float x, float y, float z);

	struct Settings;
	struct State;

	typedef std::shared_ptr<Settings> SettingsPtr;
	typedef std::shared_ptr<State> StatePtr;
	typedef std::shared_ptr<const State> StateConstPtr;

	SettingsPtr getSettings() const { return settings; }
	StateConstPtr getState() const { return state; }

	void setFirstPoint(Vector3& firstPoint);
	void setSelectionPoint(std::vector<Vector3>& selectionPoint);
	void setSelectionMatrix(std::vector<Matrix4>& selectionMatrix);
	void setPostTreatment(Vector3& postTreatmentTrans, Quaternion& postTreatmentRot);
	void setSubData(Vector3& dataTrans, Quaternion& dataRot);
	void clearSelection();
	void pushBackSelection();
private:
	unsigned int getScreenWidth() const { return SCREEN_WIDTH; }
	unsigned int getScreenHeight() const { return SCREEN_HEIGHT; }

	Matrix4 getProjMatrix() const { return projMatrix; }
	Matrix4 getOrthoProjMatrix() const { return orthoProjMatrix; }

	float getNearClipDist() const { return projNearClipDist; }
	float getFarClipDist() const { return projFarClipDist; }

	// Compute the depth value at the given distance, in NDC coordinates
	// (i.e. in the frustum defined by projMatrix)
	float getDepthValue(float dist)
	{
		const float& near = projNearClipDist;
		const float& far = projFarClipDist;
		return (far+near)/(far-near) + (1/dist) * ((-2*far*near)/(far-near));
	}

	struct Impl;
	std::unique_ptr<Impl> impl;

	Matrix4 projMatrix, orthoProjMatrix;
	float projNearClipDist, projFarClipDist;

	SettingsPtr settings;
	StatePtr state;
};

// ======================================================================
// Settings

enum SliceType {
	SLICE_CAMERA = 0, SLICE_AXIS = 1, SLICE_STYLUS = 2
};

struct FluidMechanics::Settings
{
	Settings()
	 : zoomFactor(1.0f),
	   showVolume(true),
	   showSurface(false),
	   showStylus(true),
	   showSlice(false),
	   showCrossingLines(true),
	   sliceType(SLICE_CAMERA),
	   clipDist(defaultClipDist),
	   surfacePercentage(0.13), // XXX: hardcoded testing value
	   surfacePreview(false),
	   considerX(1),
	   considerY(1),
	   considerZ(1),
	   showSelection(false)
	{}

	static constexpr float nativeZoomFactor = 2.0f; // global zoom multiplier
	static constexpr float defaultClipDist = 360.0f;

	float zoomFactor;
	bool showVolume, showSurface, showStylus, showSlice, showCrossingLines;
	SliceType sliceType;
	float clipDist; // if clipDist == 0, the clip plane is disabled
	double surfacePercentage;
	bool surfacePreview;
	short considerX = 1 ;
	short considerY = 1 ;
	short considerZ = 1 ;
	bool showSelection;
};

// ======================================================================
// State

enum ClipAxis {
	CLIP_NONE,
	CLIP_AXIS_X, CLIP_AXIS_Y, CLIP_AXIS_Z,
	CLIP_NEG_AXIS_X, CLIP_NEG_AXIS_Y, CLIP_NEG_AXIS_Z
};

struct FluidMechanics::State
{
	State()
	 : tangibleVisible(true), stylusVisible(true),
	   computedZoomFactor(1.0f),
	   clipAxis(CLIP_NONE), lockedClipAxis(CLIP_NONE)
	{}

	const bool tangibleVisible;
	const bool stylusVisible;
	float computedZoomFactor;

	ClipAxis clipAxis;
	ClipAxis lockedClipAxis;
	Synchronized<Matrix4> modelMatrix;
	Synchronized<Matrix4> sliceModelMatrix;
	Synchronized<Matrix4> stylusModelMatrix;
};

#endif /* FLUID_MECHANICS_H */
