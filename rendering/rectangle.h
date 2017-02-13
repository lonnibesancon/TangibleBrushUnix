#ifndef  RECTANGLE_INC
#define  RECTANGLE_INC

#include "global.h"
#include "renderable.h"

class Rectangle : public Renderable
{
	public:
		Rectangle(float width, float height);
		void bind();
		void render(const Matrix4& projMat, const Matrix4& mvp);
		void setSize(float w, float h);
		void setColor(const Vector3& v);

	private:
		float m_width, m_height;
		MaterialSharedPtr m_material;
		bool m_bound;
		GLint m_vertexAttrib;
		GLint m_projectionUniform, m_modelViewUniform, m_colorUniform;
		Vector3 m_color;
		float m_opacity;
};

#endif
