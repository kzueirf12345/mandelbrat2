#ifndef FPS_CHECKER_SRC_FPS_CHECKER_FPS_CHECKER_H
#define FPS_CHECKER_SRC_FPS_CHECKER_FPS_CHECKER_H

#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "sdl_objs/sdl_objs.h"

enum FPSCheckerError
{
    FPS_CHECKER_ERROR_SUCCESS           = 0,
    FPS_CHECKER_ERROR_SDL               = 1,
    FPS_CHECKER_ERROR_TTF               = 2,
    FPS_CHECKER_ERROR_STANDARD_ERRNO    = 3,
};
static_assert(FPS_CHECKER_ERROR_SUCCESS  == 0);

const char* FPS_checker_strerror(const enum FPSCheckerError error);

#define FPS_CHECKER_ERROR_HANDLE(call_func, ...)                                                    \
    do {                                                                                            \
        enum FPSCheckerError error_handler = call_func;                                            \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            FPS_checker_strerror(error_handler));                                   \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

#endif /* FPS_CHECKER_SRC_FPS_CHECKER_FPS_CHECKER_H */

enum FPSCheckerError FPS_checker_ctor(const double update_freq);
void                 FPS_checker_dtor(void);

enum FPSCheckerError FPS_checker_update(const sdl_objs_t* const sdl_objs);
enum FPSCheckerError FPS_checker_print (const sdl_objs_t* const sdl_objs);