#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "FPS_checker/FPS_checker.h"
#include "logger/liblogger.h"
#include "utils/utils.h"
#include "sdl_objs/sdl_objs.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* FPS_checker_strerror(const enum FPSCheckerError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(FPS_CHECKER_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(FPS_CHECKER_ERROR_SDL);
        CASE_ENUM_TO_STRING_(FPS_CHECKER_ERROR_TTF);
        CASE_ENUM_TO_STRING_(FPS_CHECKER_ERROR_STANDARD_ERRNO);
        default:
            return "UNKNOWN_FPS_CHECKER_ERROR";
    }
    return "UNKNOWN_FPS_CHECKER_ERROR";
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
            return FPS_CHECKER_ERROR_SDL;                                                           \
        }                                                                                           \
    } while(0)

static struct 
{
    Uint32 last_time_ms;
    size_t frame_cnt;

    double update_freq;
    double fps;
} FPS_CHECKER_ = {.last_time_ms = 0, .frame_cnt = 0, .fps = 0};

enum FPSCheckerError FPS_checker_ctor(const double update_freq)
{
    lassert((long)update_freq, "");

    FPS_CHECKER_.update_freq        = update_freq;
    FPS_CHECKER_.fps                = 0;
    FPS_CHECKER_.frame_cnt          = 0;
    FPS_CHECKER_.last_time_ms       = SDL_GetTicks();

    return FPS_CHECKER_ERROR_SUCCESS;
}

void FPS_checker_dtor(void)
{
    IF_DEBUG(FPS_CHECKER_.frame_cnt     = 0);
    IF_DEBUG(FPS_CHECKER_.last_time_ms  = 0);
    IF_DEBUG(FPS_CHECKER_.fps           = 0);
    IF_DEBUG(FPS_CHECKER_.update_freq   = 0);
}

enum FPSCheckerError FPS_checker_update(const sdl_objs_t* const sdl_objs)
{
    lassert(!is_invalid_ptr(sdl_objs), "");

    ++FPS_CHECKER_.frame_cnt;

    const Uint32 cur_time_ms = SDL_GetTicks();
    const Uint32 delta_time_ms = cur_time_ms - FPS_CHECKER_.last_time_ms;

    if (delta_time_ms >= FPS_CHECKER_.update_freq)
    {
        FPS_CHECKER_.fps = (double)FPS_CHECKER_.frame_cnt / (double)(delta_time_ms) * 1000.;

        FPS_CHECKER_.frame_cnt = 0;
        FPS_CHECKER_.last_time_ms = cur_time_ms;
    }

    FPS_CHECKER_ERROR_HANDLE(FPS_checker_print(sdl_objs));

    return FPS_CHECKER_ERROR_SUCCESS;
}

#define FPS_STR_SIZE 16
enum FPSCheckerError FPS_checker_print(const sdl_objs_t* const sdl_objs)
{
    lassert(!is_invalid_ptr(sdl_objs), "");
    lassert(!is_invalid_ptr(sdl_objs->font), "");
    lassert(!is_invalid_ptr(sdl_objs->renderer), "");

    char* const fps_str = calloc(FPS_STR_SIZE, sizeof(char));
    if (!fps_str)
    {
        perror("Can't calloc fps_str");
        return FPS_CHECKER_ERROR_STANDARD_ERRNO;
    }

    if (!snprintf(fps_str, FPS_STR_SIZE, "%.2f", FPS_CHECKER_.fps))
    {
        perror("Can't snpritnf fps to fps_str");
        free(fps_str);
        return FPS_CHECKER_ERROR_STANDARD_ERRNO;
    }

    SDL_Surface *fps_surface = TTF_RenderText_Blended(
        sdl_objs->font, 
        fps_str, 
        (SDL_Color){0, 0, 0, 255} // black
    );
    free(fps_str);

    if (!fps_surface){
        fprintf(stderr, "Can't TTF_RenderText_Blended for create fps_surface. Error: %s", 
                        TTF_GetError());
        return FPS_CHECKER_ERROR_TTF;
    }

    SDL_Texture* fps_texture = SDL_CreateTextureFromSurface(sdl_objs->renderer, fps_surface);
    SDL_FreeSurface(fps_surface);

    if (!fps_texture)
    {
        fprintf(stderr, "Can't SDL_CreateTextureFromSurface for create texture. Error: %s", 
                        SDL_GetError());
        return FPS_CHECKER_ERROR_SDL;
    }

    SDL_Rect pos = {0, 0, 0, 0};
    SDL_ERROR_HANDLE_(SDL_QueryTexture(fps_texture, NULL, NULL, &pos.w, &pos.h), 
        SDL_DestroyTexture(fps_texture);
    );

    SDL_ERROR_HANDLE_(SDL_RenderCopy(sdl_objs->renderer, fps_texture, NULL, &pos), 
        SDL_DestroyTexture(fps_texture);
    );

    SDL_DestroyTexture(fps_texture);

    return FPS_CHECKER_ERROR_SUCCESS;
}
#undef FPS_STR_SIZE