#include <SDL2/SDL.h>

#include "mandelbrat2/mandelbrat2.h"
#include "logger/liblogger.h"
#include "utils/utils.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* mandelbrat2_strerror(const enum Mandelbrat2Error error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(MANDELBRAT2_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(MANDELBRAT2_ERROR_SDL);
        default:
            return "UNKNOWN_MANDELBRAT2_ERROR";
    }
    return "UNKNOWN_MANDELBRAT2_ERROR";
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
            return MANDELBRAT2_ERROR_SDL;                                                           \
        }                                                                                           \
    } while(0)


enum Mandelbrat2Error print_frame(SDL_Texture* pixels_texture)
{
    lassert(!is_invalid_ptr(pixels_texture), "");

    const size_t    ITERS_CNT       = 256;
    const double    R_CIRCLE_INF2   = 10*10;
    const double    SCALE           = 500;

    const double    X_OFFSET        = (double)((SCREEN_WIDTH >> 1) + 100);
    const double    Y_OFFSET        = (double)( SCREEN_HEIGHT >> 1);

    void *pixels_void;
    int pitch;
    SDL_ERROR_HANDLE_(SDL_LockTexture(pixels_texture, NULL, &pixels_void, &pitch));

    Uint32* pixels = (Uint32*)pixels_void;

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            pixels[y * (pitch/4) + x] = 0xFFFFFFFF;
        }
    }

    for (size_t y_screen = 0; y_screen < SCREEN_HEIGHT; ++y_screen)
    {
        const double y0 = ((double)y_screen - Y_OFFSET) / SCALE;

        for (size_t x_screen = 0; x_screen < SCREEN_WIDTH; ++x_screen)
        {
            const double x0 = ((double)x_screen - X_OFFSET) / SCALE;

            size_t iter = 0;
            for (double x = x0, y = y0; iter < ITERS_CNT; ++iter)
            {
                const double xx = x * x;
                const double yy = y * y;
                const double xy = x * y;

                if (xx + yy > R_CIRCLE_INF2) 
                    break;
                
                x = xx - yy + x0;
                y = 2 * xy + y0;
            }

            const size_t pixel_addr = (y_screen) * (size_t)(pitch >> 2) + x_screen;
            
            if (iter == ITERS_CNT) // in circle
                pixels[pixel_addr] = 0xFF000000;
            else
                pixels[pixel_addr] = 0xFF0000FF;
        }
    }

    SDL_UnlockTexture(pixels_texture);

    return MANDELBRAT2_ERROR_SUCCESS;
}