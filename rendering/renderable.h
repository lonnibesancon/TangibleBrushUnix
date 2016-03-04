#ifndef RENDERABLE_H
#define RENDERABLE_H

#include "global.h"

class Renderable
{
public:
	virtual ~Renderable() {} // interface

	// (GL context)
	virtual void bind() = 0;

	// (GL context)
	virtual void render(const Matrix4& projectionMatrix,
	                    const Matrix4& modelViewMatrix) = 0;
};

#endif /* RENDERABLE_H */
