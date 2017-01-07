#ifndef MESH_H
#define MESH_H

#include "global.h"

#include "renderable.h"
#include "texture.h"

struct MeshData
{
	std::vector<Vector3> vertices;
	std::vector<Vector2> texCoords;
	std::vector<Vector3> normals;
	std::vector<Vector3> colors;

	// Indices for vertex/texcoord/normal values in the
	// vertices/texCoords/normals arrays
	struct Index { unsigned int v, t, n, c; };
	std::vector<Index> indices;
};

class Mesh : public Renderable
{
public:
	Mesh(const MeshData& data, TexturePtr texture = TexturePtr());

	// (GL context)
	void bind();

	// Ignored if the mesh already contains vertex colors
	void setColor(const Vector3& color);

	void setOpacity(float opacity);

	void setTexture(TexturePtr texture);

	void setShadeless(bool shadeless);
	void setShadelessColor(const Vector3& color);
	void setOnlyShadow(bool onlyShadow);

	// (GL context)
	void render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix);

	// (GL context)
	void renderShadowed(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix,
	                    const Matrix4& modelMatrix, const Matrix4& lightMatrix, GLuint shadowTexture,
						const Vector3& shadowLightPos);

private:
	// (GL context)
	void renderShaded(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix);
	// (GL context)
	void renderShadeless(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix);

	MaterialSharedPtr mMaterial, mShadelessMaterial, mShadowMaterial, mOnlyShadowMaterial;
	bool mBound, mRebindShadowShader;
	Vector3 mColor;
	float mOpacity;
	GLint mVertexAttrib, mTexCoordAttrib, mNormalAttrib, mVertexColorAttrib;
	GLint mVertexAttribShadow, mTexCoordAttribShadow, mNormalAttribShadow, mVertexColorAttribShadow;
	GLint mProjectionUniform, mModelViewUniform, mNormalMatrixUniform, mColorUniform;
	GLint mProjectionUniformShadow, mModelViewUniformShadow, mModelUniformShadow, mNormalMatrixUniformShadow, mColorUniformShadow, mLightMatrixUniformShadow;
	GLint mVertexAttribShadeless;
	GLint mProjectionUniformShadeless, mModelViewUniformShadeless, mColorUniformShadeless;
	std::vector<GLfloat> mMeshBuffer;
	unsigned int mNumFaces;
	bool mTextured;
	TexturePtr mTexture;
	bool mShadeless;
	Vector3 mShadelessColor;
	bool mOnlyShadow;
	Vector3_f minValue;
};

#endif /* MESH_H */
