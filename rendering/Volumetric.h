#ifndef  VOLUMETRIC_INC
#define  VOLUMETRIC_INC

#include "global.h"
#include "renderable.h"
#include "FillVolume.h"
#include "material.h"

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

		static MaterialSharedPtr m_material;
		static bool m_materialInit;
		bool m_bound;
		GLint m_vertexAttrib, m_normalAttrib;
		GLint m_projectionUniform, m_modelViewUniform, m_normalMatrixUniform, m_volumeUniform;
		uint32_t m_x, m_y, m_z;
};

#endif
