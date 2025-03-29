#include <xmmintrin.h>

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
        CASE_ENUM_TO_STRING_(MANDELBRAT2_ERROR_STANDARD_ERRNO);
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


enum Mandelbrat2Error mandelbrat2_state_ctor(mandelbrat2_state_t* const state, 
                                             const flags_objs_t* const flags_objs)
{
    lassert(!is_invalid_ptr(state), "");
    lassert(!is_invalid_ptr(flags_objs), "");

    state->x_offset = (double)(flags_objs->screen_width  >> 1);
    state->y_offset = (double)(flags_objs->screen_height >> 1);

    if (!fscanf(flags_objs->input_file, 
                "%*[^$]$\n"
                "iters_cnt = %zu\n"
                "r_circle_inf = %lg\n"
                "scale = %lg",
                &state->iters_cnt,
                &state->r_circle_inf,
                &state->scale
        ))
    {
        perror("Can't fscanf state into input file");
        return MANDELBRAT2_ERROR_STANDARD_ERRNO;
    }

    return MANDELBRAT2_ERROR_SUCCESS;
}

// enum Mandelbrat2Error print_frame(SDL_Texture* pixels_texture, 
//                                   const mandelbrat2_state_t* const state,
//                                   const flags_objs_t* const flags_objs)
// {
//     if (flags_objs->use_graphics)
//     {
//         lassert(!is_invalid_ptr(pixels_texture), "");
//     }   
//     lassert(!is_invalid_ptr(state), "");
//     lassert(!is_invalid_ptr(flags_objs), "");

//     const double    R_CIRCLE_INF2   = state->r_circle_inf*state->r_circle_inf;
//     const double    SCALE           = 1 / state->scale;

//     void *pixels_void = NULL;
//     int pitch = 0;

//     if (flags_objs->use_graphics)
//     {
//         SDL_ERROR_HANDLE_(SDL_LockTexture(pixels_texture, NULL, &pixels_void, &pitch));
//     }

//     Uint32* pixels = (Uint32*)pixels_void;

//     for (size_t repeat = 0; repeat < flags_objs->rep_calc_frame_cnt; ++repeat)
//     {
//         for (size_t y_screen = 0; y_screen < (size_t)flags_objs->screen_height; ++y_screen)
//         {
//             const double y0 = ((double)y_screen - state->y_offset) * SCALE;
    
//             for (size_t x_screen = 0; x_screen < (size_t)flags_objs->screen_width; ++x_screen)
//             {
//                 const double x0 = ((double)x_screen - state->x_offset) * SCALE;
    
//                 size_t iter = 0;
//                 for (double x = x0, y = y0; iter < state->iters_cnt; ++iter)
//                 {
//                     const double xx = x * x;
//                     const double yy = y * y;
//                     const double xy = x * y;
    
//                     if (xx + yy > R_CIRCLE_INF2) 
//                         break;
                    
//                     x = xx - yy + x0;
//                     y = 2 * xy + y0;
//                 }
    
//                 if (flags_objs->use_graphics)
//                 {
    
//                 const size_t pixel_addr = (y_screen) * (size_t)(pitch >> 2) + x_screen;
                
// #include SETTINGS_FILENAME  // fill pixels[pixel_addr]
                
//                 }
    
//             }
//         }
//     }

//     if (flags_objs->use_graphics)
//     {
//         SDL_UnlockTexture(pixels_texture);
//     }

//     return MANDELBRAT2_ERROR_SUCCESS;
// }

enum Mandelbrat2Error print_frame_optimize(SDL_Texture* pixels_texture, 
                                           const mandelbrat2_state_t* const state,
                                           const flags_objs_t* const flags_objs)
{
    if (flags_objs->use_graphics)
    {
        lassert(!is_invalid_ptr(pixels_texture), "");
    }   
    lassert(!is_invalid_ptr(state), "");
    lassert(!is_invalid_ptr(flags_objs), "");

    const double SCALE                  = 1 / state->scale;
    const size_t REP_CNT                = flags_objs->rep_calc_frame_cnt;
    const size_t SCREEN_HEIGHT          = (size_t)flags_objs->screen_height;
    const size_t SCREEN_WIDTH           = (size_t)flags_objs->screen_width;
    const size_t ITERS_CNT              = state->iters_cnt;

    const __m256d R_CIRCLE_INF2_VEC     = _mm256_set1_pd(state->r_circle_inf * state->r_circle_inf);
    const __m256d SCALE_VEC             = _mm256_set1_pd(SCALE);
    const __m256d X_OFFSET              = _mm256_set1_pd(state->x_offset * SCALE);
    const __m256d Y_OFFSET              = _mm256_set1_pd(state->y_offset * SCALE);

    void *pixels_void = NULL;
    int pitch = 0;

    if (flags_objs->use_graphics)
    {
        SDL_ERROR_HANDLE_(SDL_LockTexture(pixels_texture, NULL, &pixels_void, &pitch));
    }

    Uint32* pixels = (Uint32*)pixels_void;

    for (size_t repeat = 0; repeat < REP_CNT; ++repeat)
    {
        for (size_t y_screen = 0; y_screen < SCREEN_HEIGHT; ++y_screen)
        {
            __m256d y0 = _mm256_sub_pd(_mm256_set1_pd((double)y_screen * SCALE), Y_OFFSET);
    
            for (size_t x_screen = 0; x_screen < SCREEN_WIDTH; x_screen += 4)
            {
                __m256d x0 = _mm256_setr_pd(
                    (double)x_screen + 0,
                    (double)x_screen + 1,
                    (double)x_screen + 2,
                    (double)x_screen + 3
                );
                x0 = _mm256_sub_pd(_mm256_mul_pd(x0, SCALE_VEC), X_OFFSET);

                __m256d iter = _mm256_setzero_pd();
                size_t iter_cnt = 0;
                for (__m256d x = x0, y = y0; iter_cnt < ITERS_CNT ; ++iter_cnt)
                {
                    const __m256d xx = _mm256_mul_pd(x, x);
                    const __m256d yy = _mm256_mul_pd(y, y);
                    const __m256d xy = _mm256_mul_pd(x, y);

                    const __m256d cmp = _mm256_cmp_pd(
                        _mm256_add_pd(xx, yy), 
                        R_CIRCLE_INF2_VEC,
                        _CMP_LE_OQ
                    );

                    if (!_mm256_movemask_pd(cmp))
                        break;
                        
                    const __m256d mask = _mm256_and_pd(cmp, _mm256_set1_pd(1.0));

                    iter = _mm256_add_pd(iter, mask);

                    x = _mm256_add_pd(_mm256_sub_pd(xx, yy), x0);
                    y = _mm256_add_pd(_mm256_add_pd(xy, xy), y0);
                }
                
                if (flags_objs->use_graphics)
                {
                    for (size_t i = 0; i < 4; ++i)
                    {
                        const size_t pixel_addr = (y_screen) * (size_t)(pitch >> 2) + x_screen + i;
                
#include SETTINGS_FILENAME  // fill pixels[pixel_addr]
                    }         
                }
    
            }
        }
    }

    if (flags_objs->use_graphics)
    {
        SDL_UnlockTexture(pixels_texture);
    }

    return MANDELBRAT2_ERROR_SUCCESS;
}