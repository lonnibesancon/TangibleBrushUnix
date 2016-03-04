#include "texture.h"

#include <cstring>
#include <cstdio>

#include <png.h>

void user_error_fn(png_structp png_ptr, png_const_charp error_msg)
{ LOGD("PNG error: %s", error_msg); }

void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{ LOGD("PNG warning: %s", warning_msg); }

Texture::Texture(const std::string& fileName)
 : mHandle(0),
   mData(0),
   mWidth(0), mHeight(0),
   mHasAlpha(false)
{
	readPNG(fileName);
}

void Texture::readPNG(const std::string& fileName)
{
	// http://blog.nobel-joergensen.com/2010/11/07/loading-a-png-as-texture-in-opengl-using-libpng/

	png_structp png = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		// nullptr, nullptr, nullptr
		nullptr, user_error_fn, user_warning_fn
	);

	if (!png)
		throw std::runtime_error("libpng init error: " + fileName);

	png_infop info = png_create_info_struct(png);

	if (!info) {
		png_destroy_read_struct(&png, nullptr, nullptr);
		throw std::runtime_error("libpng info init error: "+ fileName);
	}

	FILE* fp;
	if (!(fp = std::fopen(fileName.c_str(), "rb")))
		throw std::runtime_error("unable to open file: " + fileName);

	if (::setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, nullptr);
		std::fclose(fp);
		throw std::runtime_error("libpng read error:" + fileName);
	}

	png_init_io(png, fp);

	unsigned int sig = 0;
	png_set_sig_bytes(png, sig);

	png_read_png(
		png, info,
		PNG_TRANSFORM_STRIP_16
		| PNG_TRANSFORM_PACKING
		| PNG_TRANSFORM_EXPAND,
		nullptr
	);

	mWidth = png_get_image_width(png, info);
	mHeight = png_get_image_height(png, info);

	switch (png_get_color_type(png, info)) {
		case PNG_COLOR_TYPE_RGBA:
			mHasAlpha = true;
			break;

		case PNG_COLOR_TYPE_RGB:
			mHasAlpha = false;
			break;

		default:
			png_destroy_read_struct(&png, &info, nullptr);
			fclose(fp);
			throw std::runtime_error("unsupported color type: " + fileName);
	}

	unsigned int rowBytes = png_get_rowbytes(png, info);
	mData.resize(rowBytes * mHeight);

	png_bytepp rows = png_get_rows(png, info);

	// Réordonne les lignes de haut en bas pour être compatible
	// avec OpenGL
	for (unsigned int i = 0; i < mHeight; ++i)
		std::memcpy(mData.data() + (rowBytes * (mHeight-1-i)), rows[i], rowBytes);

	png_destroy_read_struct(&png, &info, nullptr);
	std::fclose(fp);
}

// (GL context)
void Texture::bind()
{
	createOpenGLTexture();
}

void Texture::createOpenGLTexture()
{
	glGenTextures(1, &mHandle);

	glBindTexture(GL_TEXTURE_2D, mHandle);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,                              // base mipmap level
		(mHasAlpha ? GL_RGBA : GL_RGB), // internal storage format
		mWidth,
		mHeight,
		0,                              // border width
		(mHasAlpha ? GL_RGBA : GL_RGB), // data format
		GL_UNSIGNED_BYTE,
		mData.data()
	);

	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
	glDeleteTextures(1, &mHandle);
}
