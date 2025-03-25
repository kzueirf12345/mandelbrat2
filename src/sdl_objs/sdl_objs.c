#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "sdl_objs/sdl_objs.h"
#include "logger/liblogger.h"
#include "utils/utils.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* sdl_objs_strerror(const enum SdlObjsError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(SDL_OBJS_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(SDL_OBJS_ERROR_SDL);
        CASE_ENUM_TO_STRING_(SDL_OBJS_ERROR_TTF);
        default:
            return "UNKNOWN_SDL_OBJS_ERROR";
    }
    return "UNKNOWN_SDL_OBJS_ERROR";
}
#undef CASE_ENUM_TO_STRING_

#define SDL_ERROR_HANDLE_(call_func, ...)                                                           \
    do {                                                                                            \
        int error_handler = call_func;                                                              \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            SDL_GetError());                                                        \
            __VA_ARGS__                                                                             \
            return SDL_OBJS_ERROR_SDL;                                                              \
        }                                                                                           \
    } while(0)

#define TTF_ERROR_HANDLE_(call_func, ...)                                                           \
    do {                                                                                            \
        int error_handler = call_func;                                                              \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            TTF_GetError());                                                        \
            __VA_ARGS__                                                                             \
            return SDL_OBJS_ERROR_TTF;                                                              \
        }                                                                                           \
    } while(0)

enum SdlObjsError sdl_objs_ctor(sdl_objs_t* const sdl_objs,
                                const char * const font_filename, const int font_size,
                                const int screen_width, const int screen_height)
{
    lassert(!is_invalid_ptr(sdl_objs), "");
    lassert(!is_invalid_ptr(font_filename), "");
    lassert(font_size, "");
    lassert(screen_width, "");
    lassert(screen_height, "");

    SDL_ERROR_HANDLE_(SDL_Init(SDL_INIT_VIDEO));
    TTF_ERROR_HANDLE_(TTF_Init());

    sdl_objs->window = SDL_CreateWindow(
        "Masturbator 2000", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        screen_width, 
        screen_height, 
        SDL_WINDOW_SHOWN
    );

    if (!sdl_objs->window)
    {
        fprintf(stderr, "Can`t SDL_CreateWindow. Error: %s\n", SDL_GetError());
        SDL_Quit();
        return SDL_OBJS_ERROR_SDL;
    }

    sdl_objs->renderer = SDL_CreateRenderer(
        sdl_objs->window, 
        -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!sdl_objs->renderer)
    {
        fprintf(stderr, "Can`t SDL_CreateRenderer. Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl_objs->window);
        SDL_Quit();
        return SDL_OBJS_ERROR_SDL;
    }

    sdl_objs->pixels_texture = SDL_CreateTexture(
        sdl_objs->renderer, 
        SDL_PIXELFORMAT_RGBA32, 
        SDL_TEXTUREACCESS_STREAMING, 
        screen_width, 
        screen_height
    );

    if (!sdl_objs->pixels_texture)
    {
        fprintf(stderr, "Can`t SDL_CreateTexture. Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(sdl_objs->renderer);
        SDL_DestroyWindow(sdl_objs->window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    sdl_objs->font = TTF_OpenFont(font_filename, font_size);

    if (!sdl_objs->font)
    {
        fprintf(stderr, "Can't TTF_OpenFont. Error: %s\n", TTF_GetError());
        SDL_DestroyTexture(sdl_objs->pixels_texture);
        SDL_DestroyRenderer(sdl_objs->renderer);
        SDL_DestroyWindow(sdl_objs->window);
        SDL_Quit();
        return SDL_OBJS_ERROR_TTF;
    }

    return SDL_OBJS_ERROR_SUCCESS;
}

void sdl_objs_dtor(sdl_objs_t* const sdl_objs)
{
    TTF_CloseFont       (sdl_objs->font);
    TTF_Quit            ();
    
    SDL_DestroyTexture  (sdl_objs->pixels_texture);
    SDL_DestroyRenderer (sdl_objs->renderer);
    SDL_DestroyWindow   (sdl_objs->window);
    SDL_Quit            ();
}