#include "mesh.h"
#include "material.h"

#define NEW_SHADOWS

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
		"attribute highp vec3 vertex;\n"
		"attribute mediump vec3 normal;\n"
		"varying mediump vec3 v_normal;\n"

		"#ifdef TEXTURE\n"
		"attribute mediump vec2 texCoords;\n"
		"varying mediump vec2 v_texCoords;\n"
		"#endif\n"

		"varying lowp vec3 v_lightDir;\n"
		// "varying lowp vec3 v_lightDir2;\n"

		"#ifdef COLORS\n"
		"attribute lowp vec3 vertexColor;\n"
		"varying lowp vec3 v_vertexColor;\n"
		"#endif\n"

		"void main() {\n"
		"  v_normal = normalize(normalMatrix * normal);\n"
		"#ifdef TEXTURE\n"
		"  v_texCoords = texCoords;\n"
		"#endif\n"
		"#ifdef COLORS\n"
		"  v_vertexColor = vertexColor;\n"
		"#endif\n"
		// "  gl_Position = projection * modelView * vec4(vertex, 1.0);\n"
		"  gl_Position = projection * modelView * vec4(vertex*30.0, 1.0);\n"

		"  v_lightDir = normalize(vec3(0.0, 0.0, -1.0));\n"
		// "  v_lightDir2 = normalize(vec3(-1.0, -0.3, -0.4));\n"
		"}"
		;

	const char* fragmentShader =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		// "uniform lowp float value;\n"
		"varying mediump vec3 v_normal;\n"
		"varying lowp vec3 v_lightDir;\n"
		// "varying lowp vec3 v_lightDir2;\n"
		"#ifdef TEXTURE\n"
		"varying mediump vec2 v_texCoords;\n"
		"uniform sampler2D texture;\n"
		"#endif\n"
		"#ifdef COLORS\n"
		"varying lowp vec3 v_vertexColor;\n"
		"#endif\n"
		"uniform lowp vec4 color;\n"

		"void main() {\n"
		// "  lowp float NdotL = max(dot(normalize(v_normal), v_lightDir), 0.0);\n"
		"  lowp float NdotL = abs(dot(normalize(v_normal), v_lightDir));\n" // XXX: test
		// "  lowp float NdotL2 = max(dot(normalize(v_normal), v_lightDir2), 0.0);\n"
		// "  gl_FragColor = vec4(vec3(0.15+max(NdotL*0.65, NdotL2*0.35)), 1.0);\n"
		"#ifdef TEXTURE\n"
		"  gl_FragColor = texture2D(texture, v_texCoords);\n"
		"#else\n"
		"#ifdef COLORS\n"
		"  gl_FragColor = vec4(v_vertexColor.xyz*NdotL, color.a);\n"
		"#else\n"
		"  gl_FragColor = vec4(color.xyz*NdotL, color.a);\n"
		"#endif\n"
		"#endif\n"
		"}"
		;

	const char* vertexShaderShadeless =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform highp mat4 projection;\n"
		"uniform highp mat4 modelView;\n"
		"attribute highp vec3 vertex;\n"

		"void main() {\n"
		"  gl_Position = projection * modelView * vec4(vertex*30.0, 1.0);\n"
		"}"
		;

	const char* fragmentShaderShadeless =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		// "uniform lowp float value;\n"
		"uniform lowp vec4 color;\n"

		"void main() {\n"
		"  gl_FragColor = vec4(color.xyz, color.a);\n"
		"}"
		;

	const char* vertexShaderShadow =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform highp mat4 projection;\n"
		"uniform highp mat4 modelView;\n"
		"uniform highp mat4 model;\n"
		"uniform mediump mat3 normalMatrix;\n"
		"uniform mediump mat4 lightMatrix;\n"
#ifdef NEW_SHADOWS
		"uniform highp vec3 lightPos;\n"
#endif
		"attribute highp vec3 vertex;\n"
		"attribute mediump vec3 normal;\n"
		"varying mediump vec3 v_normal;\n"
		"varying highp vec4 v_shadowMapCoord;\n"
		"varying lowp vec3 v_lightDir;\n"

		"void main() {\n"
		// "  const highp mat4 biasMat = mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.5, 0.5, 0.0, 1.0);\n"
		// "  const highp mat4 biasMat = mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.5, 0.5, 0.0, 1.0);\n"
		"  v_normal = normalize(normalMatrix * normal);\n"
		"  gl_Position = projection * modelView * vec4(vertex*30.0, 1.0);\n"
		"  v_shadowMapCoord = lightMatrix * model * vec4(vertex*30.0, 1.0);\n"
		// "  v_shadowMapCoord = biasMat * lightMatrix * model * vec4(vertex*30.0, 1.0);\n"
#ifdef NEW_SHADOWS
		"  v_lightDir = -normalize(lightPos - (modelView * vec4(vertex*30.0, 1.0)).xyz);\n"
#else
		"  v_lightDir = normalize(vec3(0.0, 0.0, -1.0));\n"
#endif
		"}"
		;

	const char* fragmentShaderShadow =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform highp sampler2D shadowMapTex;\n"
		"varying mediump vec3 v_normal;\n"
		"varying highp vec4 v_shadowMapCoord;\n"
		"varying lowp vec3 v_lightDir;\n"
		"uniform lowp vec4 color;\n"
		// "uniform lowp vec3 shadowLightPos;\n"

		"void main() {\n"
#ifndef NEW_SHADOWS
		"  lowp float NdotL = abs(dot(normalize(v_normal), v_lightDir));\n" // XXX: test
#endif

		"  highp float depth_light = texture2DProj(shadowMapTex, v_shadowMapCoord).r;\n"

		"  highp float depth = (v_shadowMapCoord.z / v_shadowMapCoord.w);\n"
		// "  highp float depth = v_shadowMapCoord.z;\n"
		// "  highp float visibility = depth <= depth_light ? 1.0 : 0.8;\n"

		// Prevent self-shadowing issues
		// "  const highp float bias = 0.0001;\n"
		// "  const highp float bias = 0.01;\n" // OK
		"  const highp float bias = 0.002;\n"
		// "  const highp float bias = 0.0001;\n"
		// "  highp float bias = 0.0001 * abs(dot(normalize(v_normal), normalize(shadowLightPos)));\n"
		// "  highp float bias = 0.0001 * (1.0 - abs(dot(normalize(v_normal), normalize(v_shadowMapCoord.xyz))));\n"
		// https://www.opengl.org/discussion_boards/showthread.php/179571-basic-shadow-mapping-removing-artifacts?p=1244260&viewfull=1#post1244260
		// "  highp float bias = clamp(0.005*tan(acos(dot(normalize(v_normal), normalize(shadowLightPos)))), 0.0, 0.01);\n"

#ifdef NEW_SHADOWS
		// Only display shadows on fully lit faces
		"  highp float NdotL = dot(normalize(v_normal), v_lightDir);\n"
		"  highp float visibility = (depth < depth_light+bias && NdotL < 0.0 ? 1.0 : 0.75);\n"
		// Prevent aliasing on slopes
		"  visibility = mix(1.0, visibility, abs(NdotL));\n"

		"  if (NdotL < 0.0) \n"
		// "    gl_FragColor = vec4(color.xyz*(min(-dot(normalize(v_normal),v_lightDir),0.7)+0.3), color.a) * visibility;\n"
		// "    gl_FragColor = vec4(color.xyz*(-NdotL*0.7+0.3), color.a) * visibility;\n"
		"    gl_FragColor = vec4(color.xyz*(-NdotL*0.5+0.3), color.a) * visibility;\n"
		"  else\n"
		// "    gl_FragColor = vec4(color.xyz*0.3, color.a) * 0.8;\n"
		"    gl_FragColor = vec4(color.xyz*(NdotL*0.3+0.3), color.a) * visibility;\n" // add some backlighting (*NdotL) so that unlit faces are not uniformly dark
#else
		"  highp float visibility = (depth < depth_light+bias ? 1.0 : 0.8);\n"
		"  gl_FragColor = vec4(color.xyz*NdotL, color.a) * visibility;\n"
#endif

		// // Approximation: simple projection (not true shadow mapping)
		// "  gl_FragColor = vec4(color.xyz*NdotL, color.a) * (0.8+depth_light*0.2);\n"

		"}"
		;

	const char* fragmentShaderOnlyShadow =
		// "#version 100\n"
		"#ifndef GL_ES\n"
		"#define highp\n"
		"#define mediump\n"
		"#define lowp\n"
		"#endif\n"

		"uniform highp sampler2D shadowMapTex;\n"
		"varying mediump vec3 v_normal;\n"
		"varying highp vec4 v_shadowMapCoord;\n"
		"varying lowp vec3 v_lightDir;\n"
		"uniform lowp vec4 color;\n"

		"void main() {\n"
		// FIXME: lighting is not actually used
		"  lowp float NdotL = abs(dot(normalize(v_normal), v_lightDir));\n" // XXX: test
		"  highp float depth_light = texture2DProj(shadowMapTex, v_shadowMapCoord).r;\n"
		"  highp float depth = (v_shadowMapCoord.z / v_shadowMapCoord.w);\n"
		"  const highp float bias = 0.0001;\n"
#ifdef NEW_SHADOWS
		"  highp float visibility = (depth < depth_light+bias ? 0.0 : 0.3);\n"
		"  gl_FragColor = vec4(color.xyz*NdotL, 1.0)*0.0 + vec4(vec3(0.0), visibility);\n"
#else
		"  highp float visibility = (depth < depth_light+bias ? 0.0 : 0.2);\n"
		"  gl_FragColor = vec4(color.xyz*NdotL, 1.0)*0.0 + vec4(vec3(0.0), visibility);\n"
#endif
		"}"
		;
}

Mesh::Mesh(const MeshData& data, TexturePtr texture)
 : mMaterial(MaterialSharedPtr(
	             texture
				 ? new Material("#define TEXTURE\n" + std::string(vertexShader), "#define TEXTURE\n" + std::string(fragmentShader))
				 : data.colors.empty()
	             ? new Material(vertexShader, fragmentShader)
	             : new Material("#define COLORS\n" + std::string(vertexShader), "#define COLORS\n" + std::string(fragmentShader))
             )),
   mShadelessMaterial(MaterialSharedPtr(new Material(vertexShaderShadeless, fragmentShaderShadeless))),
   mShadowMaterial(MaterialSharedPtr(new Material(vertexShaderShadow, fragmentShaderShadow))),
   mOnlyShadowMaterial(MaterialSharedPtr(new Material(vertexShaderShadow, fragmentShaderOnlyShadow))),
   mBound(false), mRebindShadowShader(true),
   mColor(Vector3(1.0f)),
   mOpacity(1.0f),
   mVertexAttrib(-1), mTexCoordAttrib(-1), mNormalAttrib(-1), mVertexColorAttrib(-1),
   mVertexAttribShadow(-1), mTexCoordAttribShadow(-1), mNormalAttribShadow(-1), mVertexColorAttribShadow(-1),
   mProjectionUniform(-1), mModelViewUniform(-1), mNormalMatrixUniform(-1), mColorUniform(-1),
   mProjectionUniformShadow(-1), mModelViewUniformShadow(-1), mNormalMatrixUniformShadow(-1), mColorUniformShadow(-1), mLightMatrixUniformShadow(-1),
   mVertexAttribShadeless(-1), mProjectionUniformShadeless(-1), mModelViewUniformShadeless(-1), mColorUniformShadeless(-1),
   mNumFaces(0),
   mTexture(texture),
   mShadeless(false), mShadelessColor(Vector3(1.0f)),
   mOnlyShadow(false)
{
	android_assert(!data.indices.empty());

	for (const MeshData::Index idx : data.indices) {
		const Vector3& pos = data.vertices[idx.v];
		mMeshBuffer.push_back(pos.x);
		mMeshBuffer.push_back(pos.y);
		mMeshBuffer.push_back(pos.z);

		if (mTexture) {
			const Vector2& texCoords = data.texCoords[idx.t];
			mMeshBuffer.push_back(texCoords.x);
			mMeshBuffer.push_back(texCoords.y);
		}

		const Vector3& normal = data.normals[idx.n];
		mMeshBuffer.push_back(normal.x);
		mMeshBuffer.push_back(normal.y);
		mMeshBuffer.push_back(normal.z);

		if (!data.colors.empty()) {
			const Vector3& color = data.colors[idx.c];
			mMeshBuffer.push_back(color.x);
			mMeshBuffer.push_back(color.y);
			mMeshBuffer.push_back(color.z);
		}

		++mNumFaces;
	}
}

void Mesh::setColor(const Vector3& color)
{
	mColor = color;
}

void Mesh::setOpacity(float opacity)
{
	mOpacity = opacity;
}

void Mesh::setTexture(TexturePtr texture)
{
	mTexture = texture;
}

// (GL context)
void Mesh::bind()
{
	mMaterial->bind();

	mVertexAttrib = mMaterial->getAttribute("vertex");
	mTexCoordAttrib = mMaterial->getAttribute("texCoords");
	mNormalAttrib = mMaterial->getAttribute("normal");
	mVertexColorAttrib = mMaterial->getAttribute("vertexColor");
	mModelViewUniform = mMaterial->getUniform("modelView");
	mProjectionUniform = mMaterial->getUniform("projection");
	mNormalMatrixUniform = mMaterial->getUniform("normalMatrix");
	mColorUniform = mMaterial->getUniform("color");

	android_assert(mVertexAttrib != -1);
	// mTexCoordAttrib may be -1
	android_assert(mNormalAttrib != -1);
	// mVertexColorAttrib may be -1
	android_assert(mModelViewUniform != -1);
	android_assert(mProjectionUniform != -1);
	android_assert(mNormalMatrixUniform != -1);
	// mColorUniform may be -1

	mShadelessMaterial->bind();

	mVertexAttribShadeless = mShadelessMaterial->getAttribute("vertex");
	mModelViewUniformShadeless = mShadelessMaterial->getUniform("modelView");
	mProjectionUniformShadeless = mShadelessMaterial->getUniform("projection");
	mColorUniformShadeless = mShadelessMaterial->getUniform("color");

	android_assert(mVertexAttribShadeless != -1);
	android_assert(mModelViewUniformShadeless != -1);
	android_assert(mProjectionUniformShadeless != -1);
	android_assert(mColorUniformShadeless != -1);

	mShadowMaterial->bind();
	mOnlyShadowMaterial->bind();

	if (mTexture)
	    mTexture->bind();

	mBound = true;
}

void Mesh::setShadeless(bool shadeless)
{
	mShadeless = shadeless;
}

void Mesh::setShadelessColor(const Vector3& color)
{
	mShadelessColor = color;
}

void Mesh::setOnlyShadow(bool onlyShadow)
{
	mOnlyShadow = onlyShadow;
	mRebindShadowShader = true;
}

// (GL context)
void Mesh::render(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	if (!mBound)
		bind();

	if (mShadeless)
		renderShadeless(projectionMatrix, modelViewMatrix);
	else
		renderShaded(projectionMatrix, modelViewMatrix);
}

// (GL context)
void Mesh::renderShaded(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	glUseProgram(mMaterial->getHandle());

	unsigned int stride = 6;
	if (mTexCoordAttrib != -1)
		stride += 2;
	if (mVertexColorAttrib != -1)
		stride += 3;

	unsigned int idx = 0;

	// Vertices
	glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, false, stride*sizeof(GLfloat), &mMeshBuffer[idx]);
	glEnableVertexAttribArray(mVertexAttrib);
	idx += 3;

	if (mTexCoordAttrib != -1) {
		// Texcoords
		glVertexAttribPointer(mTexCoordAttrib, 2, GL_FLOAT, false, stride*sizeof(GLfloat), &mMeshBuffer[idx]);
		glEnableVertexAttribArray(mTexCoordAttrib);
		idx += 2;
	}

	// Normals
	glVertexAttribPointer(mNormalAttrib, 3, GL_FLOAT, false, stride*sizeof(GLfloat), &mMeshBuffer[idx]);
	glEnableVertexAttribArray(mNormalAttrib);
	idx += 3;

	if (mVertexColorAttrib != -1) {
		// Vertex colors
		glVertexAttribPointer(mVertexColorAttrib, 3, GL_FLOAT, false, stride*sizeof(GLfloat), &mMeshBuffer[idx]);
		glEnableVertexAttribArray(mVertexColorAttrib);
		idx += 3;
	}

	// Uniforms
	glUniformMatrix4fv(mProjectionUniform, 1, false, projectionMatrix.data_);
	glUniformMatrix4fv(mModelViewUniform, 1, false, modelViewMatrix.data_);
	glUniformMatrix3fv(mNormalMatrixUniform, 1, false, modelViewMatrix.inverse().transpose().get3x3Matrix().data_);

	if (mColorUniform != -1)
		glUniform4f(mColorUniform, mColor.x, mColor.y, mColor.z, mOpacity);

	if (mTexture) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mTexture->getHandle());
	}

	// Rendering
	glDrawArrays(GL_TRIANGLES, 0, mNumFaces);

	glDisableVertexAttribArray(mVertexAttrib);
	glDisableVertexAttribArray(mNormalAttrib);

	if (mTexCoordAttrib != -1)
		glDisableVertexAttribArray(mTexCoordAttrib);

	if (mVertexColorAttrib != -1)
		glDisableVertexAttribArray(mVertexColorAttrib);
}

// (GL context)
void Mesh::renderShadeless(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix)
{
	if (!mBound)
		bind();

	glUseProgram(mShadelessMaterial->getHandle());

	unsigned int stride = 6;
	if (mTexCoordAttrib != -1)
		stride += 2;
	if (mVertexColorAttrib != -1)
		stride += 3;

	unsigned int idx = 0;

	// Vertices
	glVertexAttribPointer(mVertexAttribShadeless, 3, GL_FLOAT, false, stride*sizeof(GLfloat), &mMeshBuffer[idx]);
	glEnableVertexAttribArray(mVertexAttribShadeless);
	idx += 3;

	if (mTexCoordAttrib != -1) {
		// Texcoords: ignored
		idx += 2;
	}

	// Normals: ignored
	idx += 3;

	if (mVertexColorAttrib != -1) {
		// Vertex colors: ignored
		idx += 3;
	}

	// Uniforms
	glUniformMatrix4fv(mProjectionUniformShadeless, 1, false, projectionMatrix.data_);
	glUniformMatrix4fv(mModelViewUniformShadeless, 1, false, modelViewMatrix.data_);

	glUniform4f(mColorUniformShadeless, mShadelessColor.x, mShadelessColor.y, mShadelessColor.z, 1.0f);

	// Rendering
	glDrawArrays(GL_TRIANGLES, 0, mNumFaces);

	glDisableVertexAttribArray(mVertexAttribShadeless);
}

// (GL context)
void Mesh::renderShadowed(const Matrix4& projectionMatrix, const Matrix4& modelViewMatrix,
	const Matrix4& modelMatrix, const Matrix4& lightMatrix, GLuint shadowTexture, const Vector3& shadowLightPos /* in screen space */)
{
	if (mRebindShadowShader) {
		if (!mOnlyShadow) {
			mVertexAttribShadow = mShadowMaterial->getAttribute("vertex");
			mTexCoordAttribShadow = mShadowMaterial->getAttribute("texCoords");
			mNormalAttribShadow = mShadowMaterial->getAttribute("normal");
			mVertexColorAttribShadow = mShadowMaterial->getAttribute("vertexColor");
			mModelViewUniformShadow = mShadowMaterial->getUniform("modelView");
			mModelUniformShadow = mShadowMaterial->getUniform("model");
			mProjectionUniformShadow = mShadowMaterial->getUniform("projection");
			mNormalMatrixUniformShadow = mShadowMaterial->getUniform("normalMatrix");
			mColorUniformShadow = mShadowMaterial->getUniform("color");
			mLightMatrixUniformShadow = mShadowMaterial->getUniform("lightMatrix");

		} else {
			mVertexAttribShadow = mOnlyShadowMaterial->getAttribute("vertex");
			mTexCoordAttribShadow = mOnlyShadowMaterial->getAttribute("texCoords");
			mNormalAttribShadow = mOnlyShadowMaterial->getAttribute("normal");
			mVertexColorAttribShadow = mOnlyShadowMaterial->getAttribute("vertexColor");
			mModelViewUniformShadow = mOnlyShadowMaterial->getUniform("modelView");
			mModelUniformShadow = mOnlyShadowMaterial->getUniform("model");
			mProjectionUniformShadow = mOnlyShadowMaterial->getUniform("projection");
			mNormalMatrixUniformShadow = mOnlyShadowMaterial->getUniform("normalMatrix");
			mColorUniformShadow = mOnlyShadowMaterial->getUniform("color");
			mLightMatrixUniformShadow = mOnlyShadowMaterial->getUniform("lightMatrix");
		}

		android_assert(mVertexAttribShadow != -1);
		// mTexCoordAttribShadow may be -1
		android_assert(mNormalAttribShadow != -1);
		// mVertexColorAttribShadow may be -1
		android_assert(mModelViewUniformShadow != -1);
		android_assert(mModelUniformShadow != -1);
		android_assert(mProjectionUniformShadow != -1);
		android_assert(mNormalMatrixUniformShadow != -1);
		// mColorUniformShadow may be -1
		android_assert(mLightMatrixUniformShadow != -1);

		mRebindShadowShader = false;
	}

	// glUseProgram(mShadowMaterial->getHandle());
	glUseProgram(!mOnlyShadow ? mShadowMaterial->getHandle() : mOnlyShadowMaterial->getHandle());

	unsigned int stride = 6;
	if (mTexCoordAttrib != -1)
		stride += 2;
	if (mVertexColorAttrib != -1)
		stride += 3;

	unsigned int idx = 0;

	// Vertices
	glVertexAttribPointer(mVertexAttribShadow, 3, GL_FLOAT, false, stride*sizeof(GLfloat), &mMeshBuffer[idx]);
	glEnableVertexAttribArray(mVertexAttribShadow);
	idx += 3;

	// if (mTexCoordAttribShadow != -1) { // FIXME
	// 	// Texcoords
	// 	glVertexAttribPointer(mTexCoordAttribShadow, 2, GL_FLOAT, false, stride*sizeof(GLfloat), &mMeshBuffer[idx]);
	// 	glEnableVertexAttribArray(mTexCoordAttribShadow);
	// 	idx += 2;
	// }
	if (mTexCoordAttrib != -1)
		idx += 2;

	// Normals
	glVertexAttribPointer(mNormalAttribShadow, 3, GL_FLOAT, false, stride*sizeof(GLfloat), &mMeshBuffer[idx]);
	glEnableVertexAttribArray(mNormalAttribShadow);
	idx += 3;

	// if (mVertexColorAttribShadow != -1) { // FIXME
	// 	// Vertex colors
	// 	glVertexAttribPointer(mVertexColorAttribShadow, 3, GL_FLOAT, false, stride*sizeof(GLfloat), &mMeshBuffer[idx]);
	// 	glEnableVertexAttribArray(mVertexColorAttribShadow);
	// 	idx += 3;
	// }
	if (mVertexColorAttrib != -1)
		idx += 3;

	// Uniforms
	glUniformMatrix4fv(mProjectionUniformShadow, 1, false, projectionMatrix.data_);
	glUniformMatrix4fv(mModelViewUniformShadow, 1, false, modelViewMatrix.data_);
	glUniformMatrix4fv(mModelUniformShadow, 1, false, modelMatrix.data_);
	glUniformMatrix3fv(mNormalMatrixUniformShadow, 1, false, modelViewMatrix.inverse().transpose().get3x3Matrix().data_);
	glUniformMatrix4fv(mLightMatrixUniformShadow, 1, false, lightMatrix.data_);
	// const Vector3 pos = modelViewMatrix.inverse() * shadowLightPos;
	// glUniform3f(mShadowMaterial->getUniform("shadowLightPos"), pos.x, pos.y, pos.z);

	if (mColorUniformShadow != -1)
		glUniform4f(mColorUniformShadow, mColor.x, mColor.y, mColor.z, mOpacity);

#ifdef NEW_SHADOWS
	if (!mOnlyShadow) {
		// const Vector3 pos = modelViewMatrix.inverse() * shadowLightPos;
		const Vector3 pos = shadowLightPos;
		glUniform3f(mShadowMaterial->getUniform("lightPos"), pos.x, pos.y, pos.z);
	}
#endif

	glActiveTexture(GL_TEXTURE0);
	// if (mTexture) {
	// 	glBindTexture(GL_TEXTURE_2D, mTexture->getHandle());
	// 	glActiveTexture(GL_TEXTURE1);
	// }
	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	glUniform1i(mShadowMaterial->getUniform("shadowMapTex"), 0);

	// Rendering
	glDrawArrays(GL_TRIANGLES, 0, mNumFaces);

	glDisableVertexAttribArray(mVertexAttribShadow);
	glDisableVertexAttribArray(mNormalAttribShadow);

	// FIXME
	// if (mTexCoordAttrib != -1)
	// 	glDisableVertexAttribArray(mTexCoordAttribShadow);
	// if (mVertexColorAttrib != -1)
	// 	glDisableVertexAttribArray(mVertexColorAttribShadow);
}
