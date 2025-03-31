#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "time_checker/time_checker.h"
#include "logger/liblogger.h"
#include "utils/utils.h"
#include "sdl_objs/sdl_objs.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* time_checker_strerror(const enum TimeCheckerError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(TIME_CHECKER_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(TIME_CHECKER_ERROR_SDL);
        CASE_ENUM_TO_STRING_(TIME_CHECKER_ERROR_TTF);
        CASE_ENUM_TO_STRING_(TIME_CHECKER_ERROR_STANDARD_ERRNO);
        default:
            return "UNKNOWN_TIME_CHECKER_ERROR";
    }
    return "UNKNOWN_TIME_CHECKER_ERROR";
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
            return TIME_CHECKER_ERROR_SDL;                                                           \
        }                                                                                           \
    } while(0)

static struct 
{
    size_t frame_cnt;
    size_t frame_cnt_fps;

    uint64_t last_time_tiks;
    uint64_t tiks;
    
    Uint32 last_time_fps_ms;
    double fps_update_freq;
    double FPS;
} TIME_CHECKER_ = {.last_time_fps_ms = 0, .last_time_tiks = 0, .fps_update_freq = 0, .tiks = 0,
                   .frame_cnt_fps = 0, .FPS = 0, .frame_cnt = 0};

enum TimeCheckerError time_checker_ctor(const double fps_update_freq)
{
    lassert((long)fps_update_freq, "");

    TIME_CHECKER_.last_time_tiks            = _rdtsc();
    TIME_CHECKER_.fps_update_freq           = fps_update_freq;
    TIME_CHECKER_.FPS                       = 0;
    TIME_CHECKER_.frame_cnt_fps             = 0;
    TIME_CHECKER_.tiks                      = 0;
    TIME_CHECKER_.frame_cnt                 = 0;
    TIME_CHECKER_.last_time_fps_ms          = SDL_GetTicks();

    return TIME_CHECKER_ERROR_SUCCESS;
}

void time_checker_dtor(void)
{
    IF_DEBUG(TIME_CHECKER_.last_time_tiks       = 0);
    IF_DEBUG(TIME_CHECKER_.fps_update_freq      = 0);
    IF_DEBUG(TIME_CHECKER_.FPS                  = 0);
    IF_DEBUG(TIME_CHECKER_.frame_cnt_fps        = 0);
    IF_DEBUG(TIME_CHECKER_.tiks                 = 0);
    IF_DEBUG(TIME_CHECKER_.frame_cnt            = 0);
    IF_DEBUG(TIME_CHECKER_.last_time_fps_ms     = 0);
}

enum TimeCheckerError time_checker_update(const sdl_objs_t* const sdl_objs)
{
    lassert(!is_invalid_ptr(sdl_objs), "");

    const uint64_t cur_time_tiks = _rdtsc();
    TIME_CHECKER_.tiks = cur_time_tiks - TIME_CHECKER_.last_time_tiks;

    ++TIME_CHECKER_.frame_cnt_fps;
    ++TIME_CHECKER_.frame_cnt;

    const Uint32 cur_time_ms    = SDL_GetTicks();
    const Uint32 delta_time_ms  = cur_time_ms - TIME_CHECKER_.last_time_fps_ms;

    if (delta_time_ms >= TIME_CHECKER_.fps_update_freq)
    {
        TIME_CHECKER_.FPS = (double)TIME_CHECKER_.frame_cnt_fps / (double)(delta_time_ms) * 1000.;

        TIME_CHECKER_.frame_cnt_fps = 0;
        TIME_CHECKER_.last_time_fps_ms = cur_time_ms;
    }

    TIME_CHECKER_ERROR_HANDLE(time_checker_print(sdl_objs));

    TIME_CHECKER_.last_time_tiks = cur_time_tiks;

    return TIME_CHECKER_ERROR_SUCCESS;
}

#define TIME_STR_SIZE 16
enum TimeCheckerError time_checker_print(const sdl_objs_t* const sdl_objs)
{
    lassert(!is_invalid_ptr(sdl_objs), "");
    lassert(!is_invalid_ptr(sdl_objs->font), "");
    lassert(!is_invalid_ptr(sdl_objs->renderer), "");

    printf("Time for calculate %zu frame: %zu tiks\n", TIME_CHECKER_.frame_cnt, TIME_CHECKER_.tiks);

    char* const TIME_str = calloc(TIME_STR_SIZE, sizeof(char));
    if (!TIME_str)
    {
        perror("Can't calloc TIME_str");
        return TIME_CHECKER_ERROR_STANDARD_ERRNO;
    }

    if (!snprintf(TIME_str, TIME_STR_SIZE, "%.2f", TIME_CHECKER_.FPS))
    {
        perror("Can't snpritnf FPS to TIME_str");
        free(TIME_str);
        return TIME_CHECKER_ERROR_STANDARD_ERRNO;
    }

    SDL_Surface *TIME_surface = TTF_RenderText_Blended(
        sdl_objs->font, 
        TIME_str, 
        (SDL_Color){255, 255, 255, 255} // black
    );
    free(TIME_str);

    if (!TIME_surface){
        fprintf(stderr, "Can't TTF_RenderText_Blended for create TIME_surface. Error: %s", 
                        TTF_GetError());
        return TIME_CHECKER_ERROR_TTF;
    }

    SDL_Texture* TIME_texture = SDL_CreateTextureFromSurface(sdl_objs->renderer, TIME_surface);
    SDL_FreeSurface(TIME_surface);

    if (!TIME_texture)
    {
        fprintf(stderr, "Can't SDL_CreateTextureFromSurface for create texture. Error: %s", 
                        SDL_GetError());
        return TIME_CHECKER_ERROR_SDL;
    }

    SDL_Rect pos = {0, 0, 0, 0};
    SDL_ERROR_HANDLE_(SDL_QueryTexture(TIME_texture, NULL, NULL, &pos.w, &pos.h), 
        SDL_DestroyTexture(TIME_texture);
    );

    SDL_ERROR_HANDLE_(SDL_RenderCopy(sdl_objs->renderer, TIME_texture, NULL, &pos), 
        SDL_DestroyTexture(TIME_texture);
    );

    SDL_DestroyTexture(TIME_texture);

    return TIME_CHECKER_ERROR_SUCCESS;
}
#undef TIME_STR_SIZE