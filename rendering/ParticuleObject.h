#ifndef  PARTICULEOBJECT_INC
#define  PARTICULEOBJECT_INC

#include "rendering/renderable.h"
#include "global.h"
#include "FillVolume.h"


struct ParticuleStats
{
	int valid=0;
	int inNoise=0;
	int incorrect=0;
	int nbParticule=0;
	int volume=0;
};

class ParticuleObject : public Renderable
{
	public:
		ParticuleObject(const std::string& fileStats, const std::string& filePoints);
		~ParticuleObject();
		virtual void bind();
		virtual void render(const Matrix4& projectionMatrix,
							const Matrix4& modelViewMatrix);

		void setClipPlane(float a, float b, float c, float d); // plane equation: ax+by+cz+d=0
		void clearClipPlane();

		Vector3 getSize() const{return mMax - mMin;}
		Vector3 getMiddle() const{return (mMax + mMin)*0.5;}
		void getStats(ParticuleStats* ps, FillVolume* fv);
	private:
		bool hasClipPlane();

		// (GL context)
		void initXPlanes(unsigned int& baseIndex);
		void initYPlanes(unsigned int& baseIndex);
		void initZPlanes(unsigned int& baseIndex);


		MaterialSharedPtr mMaterial;
		GLint mVertexAttrib, mSliceAttrib, mProjectionUniform, mModelViewUniform, mColorUniform, mDimensionsUniform, mStatusAttrib;

		bool mBound;
		float mClipEq[4];
		float mColor[4];

		float* mPoints;
		int*   mPointsStats;
		uint32_t mNbParticules;

		Vector3 mMin;
		Vector3 mMax;
};

#endif
