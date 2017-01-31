#ifndef  VOLUMETRIC_INC
#define  VOLUMETRIC_INC

#include "global.h"
#include "renderable.h"
#include "FillVolume.h"

class Volumetric : public Renderable
{
	public:
		Volumetric(FillVolume* fill, const Vector3_f& c, float alpha);
		~Volumetric();
		void bind();

		void render(const Matrix4& projectionMatrix,
					const Matrix4& modelViewMatrix);
	private:
		GLuint m_textureId;

		MaterialSharedPtr m_material;
		bool m_bound;
		GLint m_vertexAttrib, m_normalAttrib;
		GLint m_projectionUniform, m_modelViewUniform, m_normalMatrixUniform, m_volumeUniform;
};

#endif
