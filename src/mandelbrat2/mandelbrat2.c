#include <xmmintrin.h>
#include <omp.h>

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

    state->x_offset = (float)(flags_objs->screen_width  >> 1);
    state->y_offset = (float)(flags_objs->screen_height >> 1);

    if (!fscanf(flags_objs->input_file, 
                "%*[^$]$\n"
                "iters_cnt = %zu\n"
                "r_circle_inf = %f\n"
                "scale = %f",
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

#ifndef __AVX2__

enum Mandelbrat2Error print_frame(SDL_Texture* pixels_texture, 
                                  const mandelbrat2_state_t* const state,
                                  const flags_objs_t* const flags_objs)
{
    if (flags_objs->use_graphics)
    {
        lassert(!is_invalid_ptr(pixels_texture), "");
    }   
    lassert(!is_invalid_ptr(state), "");
    lassert(!is_invalid_ptr(flags_objs), "");

    const double    R_CIRCLE_INF2   = state->r_circle_inf*state->r_circle_inf;
    const double    SCALE           = 1 / state->scale;
    const size_t    ITERS_CNT       = state->iters_cnt;

    void *pixels_void = NULL;
    int pitch = 0;

    if (flags_objs->use_graphics)
    {
        SDL_ERROR_HANDLE_(SDL_LockTexture(pixels_texture, NULL, &pixels_void, &pitch));
    }

    Uint32* pixels = (Uint32*)pixels_void;

    for (size_t repeat = 0; repeat < flags_objs->rep_calc_frame_cnt; ++repeat)
    {
        for (size_t y_screen = 0; y_screen < (size_t)flags_objs->screen_height; ++y_screen)
        {
            const double y0 = ((double)y_screen - state->y_offset) * SCALE;
    
            for (size_t x_screen = 0; x_screen < (size_t)flags_objs->screen_width; ++x_screen)
            {
                const double x0 = ((double)x_screen - state->x_offset) * SCALE;
    
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
    
                if (flags_objs->use_graphics)
                {
    
                    const size_t pixel_addr = (y_screen) * (size_t)(pitch >> 2) + x_screen;
                
#include SETTINGS_FILENAME  // fill pixels[pixel_addr]
                
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

#else /*__AVX2__*/

#ifndef __aligned
#define __aligned __attribute__((aligned(32)))
#endif

#define UNROLL_CNT 4
#define SIMD_OBJS_CNT 8 
enum Mandelbrat2Error print_frame(SDL_Texture* pixels_texture, 
                                  const mandelbrat2_state_t* const state,
                                  const flags_objs_t* const flags_objs)
{
    if (flags_objs->use_graphics)
    {
        lassert(!is_invalid_ptr(pixels_texture), "");
    }   
    lassert(!is_invalid_ptr(state), "");
    lassert(!is_invalid_ptr(flags_objs), "");

    const float SCALE           = 1.0f / state->scale;
    const size_t REP_CNT        = flags_objs->rep_calc_frame_cnt;
    const size_t SCREEN_HEIGHT  = (size_t)flags_objs->screen_height;
    const size_t SCREEN_WIDTH   = (size_t)flags_objs->screen_width - (SIMD_OBJS_CNT - 1);
    const size_t ITERS_CNT      = state->iters_cnt;

    const __m256 R_CIRCLE_INF2_VEC  = _mm256_set1_ps(state->r_circle_inf * state->r_circle_inf);
    const __m256 SCALE_VEC          = _mm256_set1_ps(SCALE);
    const __m256 X_OFFSET           = _mm256_set1_ps(state->x_offset * SCALE);
    const __m256 Y_OFFSET           = _mm256_set1_ps(state->y_offset * SCALE);
    const __m256 ONE                = _mm256_set1_ps(1.0f);
    const __m256 TWO                = _mm256_set1_ps(2.0f);
    const __m256 NATURAL08          = _mm256_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);

    void *pixels_void __aligned = NULL;
    int pitch = 0;

    if (flags_objs->use_graphics)
    {
        SDL_ERROR_HANDLE_(SDL_LockTexture(pixels_texture, NULL, &pixels_void, &pitch));
    }

    Uint32* pixels __aligned = (Uint32*)pixels_void;

#pragma omp parallel for collapse(3) schedule(guided)

    for (size_t repeat = 0; repeat < REP_CNT; ++repeat)
    {
        for (size_t y_screen = 0; y_screen < SCREEN_HEIGHT; ++y_screen)
        {
            __m256 y0 = _mm256_sub_ps(_mm256_set1_ps((float)y_screen * SCALE), Y_OFFSET);
    
            for (size_t x_screen = 0; x_screen < SCREEN_WIDTH; x_screen += SIMD_OBJS_CNT)
            {
                __m256 x0 = _mm256_add_ps(NATURAL08, _mm256_set1_ps((float)x_screen));
                       x0 = _mm256_sub_ps(_mm256_mul_ps(x0, SCALE_VEC), X_OFFSET);

                __m256 iter = _mm256_setzero_ps();
                __m256 x = x0, y = y0;

                for (size_t i = 0; i < ITERS_CNT; ++i) {
                    __m256 xx = _mm256_mul_ps(x, x);
                    __m256 yy = _mm256_mul_ps(y, y);
                    __m256 xy = _mm256_mul_ps(x, y);
                    
                    __m256 cmp = _mm256_cmp_ps(_mm256_add_ps(xx, yy), R_CIRCLE_INF2_VEC, _CMP_LE_OQ);

                    if (_mm256_testz_ps(cmp, cmp)) 
                        break;
                    
                    iter = _mm256_add_ps(iter, _mm256_and_ps(cmp, ONE));
                    x = _mm256_add_ps(_mm256_sub_ps(xx, yy), x0);
                    y = _mm256_fmadd_ps(xy, TWO, y0);
                }

                if (flags_objs->use_graphics)
                {
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i)
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



#endif /*__AVX2__*/