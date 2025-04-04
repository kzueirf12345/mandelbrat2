#ifndef TIME_CHECKER_SRC_TIME_CHECKER_TIME_CHECKER_H
#define TIME_CHECKER_SRC_TIME_CHECKER_TIME_CHECKER_H

#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "sdl_objs/sdl_objs.h"

enum TimeCheckerError
{
    TIME_CHECKER_ERROR_SUCCESS           = 0,
    TIME_CHECKER_ERROR_SDL               = 1,
    TIME_CHECKER_ERROR_TTF               = 2,
    TIME_CHECKER_ERROR_STANDARD_ERRNO    = 3,
};
static_assert(TIME_CHECKER_ERROR_SUCCESS  == 0);

const char* time_checker_strerror(const enum TimeCheckerError error);

#define TIME_CHECKER_ERROR_HANDLE(call_func, ...)                                                   \
    do {                                                                                            \
        enum TimeCheckerError error_handler = call_func;                                            \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            time_checker_strerror(error_handler));                                  \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

#endif /* TIME_CHECKER_SRC_TIME_CHECKER_TIME_CHECKER_H */

enum TimeCheckerError time_checker_ctor(const double fps_update_freq, const bool use_graphics,
                                        const char* const output_filename);
enum TimeCheckerError time_checker_dtor(void);

enum TimeCheckerError time_checker_update(const sdl_objs_t* const sdl_objs);
enum TimeCheckerError time_checker_print (const sdl_objs_t* const sdl_objs);