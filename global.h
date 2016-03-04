#ifndef GLOBAL_H
#define GLOBAL_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <utility>
#include <stdexcept>
#include <cassert>

#define android_assert assert

#define LOGD(...) (printf(__VA_ARGS__), printf("\n"))
#define LOGI(...) (printf(__VA_ARGS__), printf("\n"))
#define LOGW(...) (fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"))
#define LOGE(...) (fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"))

// #define DOUBLE_PRECISION
#include "util/linear_math.h"

#include "util/synchronized.h"
#include "util/utility.h"

#define GL_GLEXT_PROTOTYPES
// #include <GLES2/gl2.h>
// #include <GLES2/gl2ext.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "fwd.h"

static const int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

#endif /* GLOBAL_H */
