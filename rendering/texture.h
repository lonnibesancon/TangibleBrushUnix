#ifndef TEXTURE_H
#define TEXTURE_H

#include "global.h"

class Texture
{
public:
	static TexturePtr createFromFile(const std::string& fileName)
	{ return TexturePtr(new Texture(fileName)); }

	virtual ~Texture();

	// (GL context)
	void bind();

	GLuint getHandle() const { return mHandle; }

	unsigned int getWidth() const { return mWidth; }
	unsigned int getHeight() const { return mHeight; }

	bool hasAlpha() const { return mHasAlpha; }

private:
	Texture(const std::string& fileName);

	void readPNG(const std::string& fileName);
	void createOpenGLTexture();

	GLuint mHandle;
	std::vector<unsigned char> mData;
	unsigned int mWidth, mHeight;
	bool mHasAlpha;
};

#endif /* TEXTURE_H */
