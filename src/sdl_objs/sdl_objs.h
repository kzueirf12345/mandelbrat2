#ifndef SDL_OBJS_SRC_SDL_OBJS_SDL_OBJS_H
#define SDL_OBJS_SRC_SDL_OBJS_SDL_OBJS_H

#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "flags/flags.h"
#include "mandelbrat2/mandelbrat2.h"

enum SdlObjsError
{
    SDL_OBJS_ERROR_SUCCESS          = 0,
    SDL_OBJS_ERROR_SDL              = 1,
    SDL_OBJS_ERROR_TTF              = 2,
};
static_assert(SDL_OBJS_ERROR_SUCCESS  == 0, "");

const char* sdl_objs_strerror(const enum SdlObjsError error);

#define SDL_OBJS_ERROR_HANDLE(call_func, ...)                                                    \
    do {                                                                                            \
        enum SdlObjsError error_handler = call_func;                                            \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            sdl_objs_strerror(error_handler));                                   \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

typedef struct SdlObjs
{
    SDL_Window*     window;
    SDL_Renderer*   renderer;
    SDL_Texture*    pixels_texture;

    TTF_Font*       font;
} sdl_objs_t;

enum SdlObjsError sdl_objs_ctor(sdl_objs_t* const sdl_objs, const flags_objs_t * const flags_objs);
void              sdl_objs_dtor(sdl_objs_t* const sdl_objs);

enum SdlObjsError sdl_handle_events(SDL_Event* event, const flags_objs_t * const flags_objs,
                                    mandelbrat2_state_t* const state, SDL_bool* const quit);

#endif /* SDL_OBJS_SRC_SDL_OBJS_SDL_OBJS_H */