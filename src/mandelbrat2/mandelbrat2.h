#ifndef MANDELBRAT2_SRC_MANDELBRAT2_MANDELBRAT2_H
#define MANDELBRAT2_SRC_MANDELBRAT2_MANDELBRAT2_H

#include <assert.h>

#include <SDL2/SDL.h>

enum Mandelbrat2Error
{
    MANDELBRAT2_ERROR_SUCCESS       = 0,
    MANDELBRAT2_ERROR_SDL           = 1,
};
static_assert(MANDELBRAT2_ERROR_SUCCESS  == 0);

const char* mandelbrat2_strerror(const enum Mandelbrat2Error error);

#define MANDELBRAT2_ERROR_HANDLE(call_func, ...)                                                    \
    do {                                                                                            \
        enum Mandelbrat2Error error_handler = call_func;                                            \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            mandelbrat2_strerror(error_handler));                                   \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

enum Mandelbrat2Error print_frame(SDL_Texture* pixels);

#endif /* MANDELBRAT2_SRC_MANDELBRAT2_MANDELBRAT2_H */