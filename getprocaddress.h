#ifndef GETPROCADDRESS_H
#define GETPROCADDRESS_H

#include <SDL2/SDL_video.h>

#pragma GCC system_header

template <typename Result>
Result GetProcAddress(const char* name)
{
    return reinterpret_cast<Result>(SDL_GL_GetProcAddress(name));
}

#endif /* GETPROCADDRESS_H */
