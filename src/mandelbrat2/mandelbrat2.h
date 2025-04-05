#ifndef MANDELBRAT2_SRC_MANDELBRAT2_MANDELBRAT2_H
#define MANDELBRAT2_SRC_MANDELBRAT2_MANDELBRAT2_H

#include <assert.h>

#include <SDL2/SDL.h>

#include "flags/flags.h"

enum Mandelbrat2Error
{
    MANDELBRAT2_ERROR_SUCCESS           = 0,
    MANDELBRAT2_ERROR_SDL               = 1,
    MANDELBRAT2_ERROR_STANDARD_ERRNO    = 2,
};
static_assert(MANDELBRAT2_ERROR_SUCCESS  == 0, "");

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

typedef struct Mandelbrat2State
{
    size_t iters_cnt;
    float r_circle_inf;

    float scale;
    float x_offset;
    float y_offset;

} mandelbrat2_state_t;

enum Mandelbrat2Error mandelbrat2_state_ctor(mandelbrat2_state_t* const state, 
                                             const flags_objs_t* const flags_objs);

enum Mandelbrat2Error print_frame(SDL_Texture* pixels_texture, 
                                  const mandelbrat2_state_t* const state,
                                  const flags_objs_t* const flags_objs);


#endif /* MANDELBRAT2_SRC_MANDELBRAT2_MANDELBRAT2_H */