#ifndef MANDELBRAT2_SRC_UTILS_UTILS_H
#define MANDELBRAT2_SRC_UTILS_UTILS_H

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <SDL2/SDL.h>

#include "concole.h"

#define DEFAULT_SCREEN_WIDTH    2048
#define DEFAULT_SCREEN_HEIGHT   1024

#define CENTERED_WINDOW_OPT     -1

#define FPS_FREQ_MS             100

#ifndef SETTINGS_FILENAME
#define SETTINGS_FILENAME      "settings.txt"
#endif /*SETTINGS_FILENAME*/

#define OFFSET_STEP       20
#define SCALE_STEP        0.1f

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifndef NDEBUG
#define IF_DEBUG(...) __VA_ARGS__
#define IF_ELSE_DEBUG(smth, other_smth) smth
#else /*NDEBUG*/
#define IF_DEBUG(...)
#define IF_ELSE_DEBUG(smth, other_smth) other_smth
#endif /*NDEBUG*/

#define INT_ERROR_HANDLE(call_func, ...)                                                            \
    do {                                                                                            \
        int error_handler = call_func;                                                              \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Errno: %d\n",                                     \
                            errno);                                                                 \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

#define SDL_ERROR_HANDLE(call_func, ...)                                                            \
    do {                                                                                            \
        int error_handler = call_func;                                                              \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            SDL_GetError());                                                        \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

enum PtrState
{
    PTR_STATES_VALID   = 0,
    PTR_STATES_NULL    = 1,
    PTR_STATES_INVALID = 2,
    PTR_STATES_ERROR   = 3
};
static_assert(PTR_STATES_VALID == 0, "");

enum PtrState is_invalid_ptr(const void* ptr);

int is_empty_file (FILE* file);

#endif /*MANDELBRAT2_SRC_UTILS_UTILS_H*/