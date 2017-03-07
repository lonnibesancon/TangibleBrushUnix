#ifndef  PARTICULEOBJECT_INC
#define  PARTICULEOBJECT_INC

#include "rendering/renderable.h"
#include "global.h"

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
	private:
		bool hasClipPlane();

		// (GL context)
		void initXPlanes(unsigned int& baseIndex);
		void initYPlanes(unsigned int& baseIndex);
		void initZPlanes(unsigned int& baseIndex);


		MaterialSharedPtr mMaterial;
		GLint mVertexAttrib, mSliceAttrib, mProjectionUniform, mModelViewUniform, mColorUniform;

		bool mBound;
		float mClipEq[4];
		float mColor[4];

		float* mPoints;
		char*  mPointsStats;
		uint32_t mNbParticules;

		Vector3 mMin;
		Vector3 mMax;
};

#endif
