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

// #ifdef COMPILE_OPTIMIZED
// #pragma omp parallel for collapse(1) schedule(guided)
// #endif /*COMPILE_OPTIMIZED*/

        for (size_t y_screen = 0; y_screen < (size_t)flags_objs->screen_height; ++y_screen)
        {
            const double y0 = ((double)y_screen - state->y_offset) * SCALE;
    
            for (size_t x_screen = 0; x_screen < (size_t)flags_objs->screen_width; ++x_screen)
            {
                const double x0 = ((double)x_screen - state->x_offset) * SCALE;
    
                volatile size_t iter = 0;
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

#ifdef X86

#ifdef UNROLL

#define Y0_CTOR4_                                                                                   \
    __m256 y01 = _mm256_sub_ps(_mm256_set1_ps((float)y_screen * SCALE), Y_OFFSET);                  \
    __m256 y02 = y01;                                                                               \
    __m256 y03 = y01;                                                                               \
    __m256 y04 = y01;

#define X0_CTOR4_                                                                                   \
    __m256 x01 = _mm256_add_ps(NATURAL08, _mm256_set1_ps((float)x_screen + 8*0));                   \
    __m256 x02 = _mm256_add_ps(NATURAL08, _mm256_set1_ps((float)x_screen + 8*1));                   \
    __m256 x03 = _mm256_add_ps(NATURAL08, _mm256_set1_ps((float)x_screen + 8*2));                   \
    __m256 x04 = _mm256_add_ps(NATURAL08, _mm256_set1_ps((float)x_screen + 8*3));                   \
           x01 = _mm256_sub_ps(_mm256_mul_ps(x01, SCALE_VEC), X_OFFSET);                            \
           x02 = _mm256_sub_ps(_mm256_mul_ps(x02, SCALE_VEC), X_OFFSET);                            \
           x03 = _mm256_sub_ps(_mm256_mul_ps(x03, SCALE_VEC), X_OFFSET);                            \
           x04 = _mm256_sub_ps(_mm256_mul_ps(x04, SCALE_VEC), X_OFFSET);                            

#define ITER_CTOR4_                                                                                 \
    volatile __m256 iter1 = _mm256_setzero_ps();                                                    \
    volatile __m256 iter2 = _mm256_setzero_ps();                                                    \
    volatile __m256 iter3 = _mm256_setzero_ps();                                                    \
    volatile __m256 iter4 = _mm256_setzero_ps();                                                             

#define X_CTOR4_                                                                                    \
    __m256 x1 = x01;                                                                                \
    __m256 x2 = x02;                                                                                \
    __m256 x3 = x03;                                                                                \
    __m256 x4 = x04;                                                                                

#define Y_CTOR4_                                                                                    \
    __m256 y1 = y01;                                                                                \
    __m256 y2 = y02;                                                                                \
    __m256 y3 = y03;                                                                                \
    __m256 y4 = y04;                                                                                

#define XX_CTOR4_                                                                                   \
    __m256 xx1 = _mm256_mul_ps(x1, x1);                                                             \
    __m256 xx2 = _mm256_mul_ps(x2, x2);                                                             \
    __m256 xx3 = _mm256_mul_ps(x3, x3);                                                             \
    __m256 xx4 = _mm256_mul_ps(x4, x4);                                                             

#define YY_CTOR4_                                                                                   \
    __m256 yy1 = _mm256_mul_ps(y1, y1);                                                             \
    __m256 yy2 = _mm256_mul_ps(y2, y2);                                                             \
    __m256 yy3 = _mm256_mul_ps(y3, y3);                                                             \
    __m256 yy4 = _mm256_mul_ps(y4, y4);                                                             

#define XY_CTOR4_                                                                                   \
    __m256 xy1 = _mm256_mul_ps(x1, y1);                                                             \
    __m256 xy2 = _mm256_mul_ps(x2, y2);                                                             \
    __m256 xy3 = _mm256_mul_ps(x3, y3);                                                             \
    __m256 xy4 = _mm256_mul_ps(x4, y4);                                                             

#define CMP_CTOR4_                                                                                  \
    __m256 cmp1 = _mm256_cmp_ps(_mm256_add_ps(xx1, yy1), R_CIRCLE_INF2_VEC, _CMP_LE_OQ);            \
    __m256 cmp2 = _mm256_cmp_ps(_mm256_add_ps(xx2, yy2), R_CIRCLE_INF2_VEC, _CMP_LE_OQ);            \
    __m256 cmp3 = _mm256_cmp_ps(_mm256_add_ps(xx3, yy3), R_CIRCLE_INF2_VEC, _CMP_LE_OQ);            \
    __m256 cmp4 = _mm256_cmp_ps(_mm256_add_ps(xx4, yy4), R_CIRCLE_INF2_VEC, _CMP_LE_OQ);  

#define CHECK_CMP4_                                                                                 \
    (                                                                                               \
        _mm256_testz_ps(cmp1, cmp1)                                                                 \
     && _mm256_testz_ps(cmp2, cmp2)                                                                 \
     && _mm256_testz_ps(cmp3, cmp3)                                                                 \
     && _mm256_testz_ps(cmp4, cmp4)                                                                 \
    )

#define UPDATE_ITER4_                                                                               \
    iter1 = _mm256_add_ps(iter1, _mm256_and_ps(cmp1, ONE));                                         \
    iter2 = _mm256_add_ps(iter2, _mm256_and_ps(cmp2, ONE));                                         \
    iter3 = _mm256_add_ps(iter3, _mm256_and_ps(cmp3, ONE));                                         \
    iter4 = _mm256_add_ps(iter4, _mm256_and_ps(cmp4, ONE));

#define UPDATE_X4_                                                                                  \
    x1 = _mm256_add_ps(_mm256_sub_ps(xx1, yy1), x01);                                               \
    x2 = _mm256_add_ps(_mm256_sub_ps(xx2, yy2), x02);                                               \
    x3 = _mm256_add_ps(_mm256_sub_ps(xx3, yy3), x03);                                               \
    x4 = _mm256_add_ps(_mm256_sub_ps(xx4, yy4), x04);

#define UPDATE_Y4_                                                                                  \
    y1 = _mm256_fmadd_ps(xy1, TWO, y01);                                                            \
    y2 = _mm256_fmadd_ps(xy2, TWO, y02);                                                            \
    y3 = _mm256_fmadd_ps(xy3, TWO, y03);                                                            \
    y4 = _mm256_fmadd_ps(xy4, TWO, y04);


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
    const size_t SCREEN_WIDTH   = (size_t)flags_objs->screen_width - (SIMD_OBJS_CNT*UNROLL_CNT - 1);
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

    
    for (size_t repeat = 0; repeat < REP_CNT; ++repeat)
    {

// #ifdef COMPILE_OPTIMIZED
// #pragma omp parallel for collapse(1) schedule(guided)
// #endif /*COMPILE_OPTIMIZED*/

        for (size_t y_screen = 0; y_screen < SCREEN_HEIGHT; ++y_screen)
        {
            Y0_CTOR4_
    
            for (size_t x_screen = 0; x_screen < SCREEN_WIDTH; x_screen += SIMD_OBJS_CNT*UNROLL_CNT)
            {
                X0_CTOR4_
                
                ITER_CTOR4_
                X_CTOR4_
                Y_CTOR4_

                for (size_t i = 0; i < ITERS_CNT; ++i) {
                    XX_CTOR4_
                    YY_CTOR4_
                    XY_CTOR4_
                    
                    CMP_CTOR4_

                    if (CHECK_CMP4_) 
                        break;
                    
                    UPDATE_ITER4_
                    UPDATE_X4_
                    UPDATE_Y4_
                }

                if (flags_objs->use_graphics)
                {
                    float iter[UNROLL_CNT*SIMD_OBJS_CNT] = {};
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter[i + SIMD_OBJS_CNT*(1-1)] = iter1[i] ;}
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter[i + SIMD_OBJS_CNT*(2-1)] = iter2[i] ;}
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter[i + SIMD_OBJS_CNT*(3-1)] = iter3[i] ;}
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter[i + SIMD_OBJS_CNT*(4-1)] = iter4[i] ;}

                    for (size_t i = 0; i < SIMD_OBJS_CNT*UNROLL_CNT; ++i)
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

#else /*UNROLL*/

#ifndef __aligned
#define __aligned __attribute__((aligned(32)))
#endif

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
    const size_t SCREEN_WIDTH   = (size_t)flags_objs->screen_width - (SIMD_OBJS_CNT- 1);
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

    
    for (size_t repeat = 0; repeat < REP_CNT; ++repeat)
    {

// #ifdef COMPILE_OPTIMIZED
// #pragma omp parallel for collapse(1) schedule(guided)
// #endif /*COMPILE_OPTIMIZED*/

        for (size_t y_screen = 0; y_screen < SCREEN_HEIGHT; ++y_screen)
        {
            __m256 y0 = _mm256_sub_ps(_mm256_set1_ps((float)y_screen * SCALE), Y_OFFSET);
    
            for (size_t x_screen = 0; x_screen < SCREEN_WIDTH; x_screen += SIMD_OBJS_CNT)
            {
                __m256 x0 = _mm256_add_ps(NATURAL08, _mm256_set1_ps((float)x_screen + 8*0));
                       x0 = _mm256_sub_ps(_mm256_mul_ps(x0, SCALE_VEC), X_OFFSET); 
                
                volatile __m256 iter = _mm256_setzero_ps(); 
                __m256 x = x0;
                __m256 y = y0;

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

#endif /*UNROLL*/

#else /*X86*/

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
    const size_t SCREEN_WIDTH   = (size_t)flags_objs->screen_width - (SIMD_OBJS_CNT*UNROLL_CNT - 1);
    const size_t ITERS_CNT      = state->iters_cnt;

    float R_CIRCLE_INF2_VEC[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd 
    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { R_CIRCLE_INF2_VEC[i] = state->r_circle_inf * state->r_circle_inf; }
    
    float SCALE_VEC[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd 
    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { SCALE_VEC[i] = SCALE; }
    
    float X_OFFSET[SIMD_OBJS_CNT]  __aligned = {}; 
#pragma omp simd 
    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { X_OFFSET[i] = state->x_offset * SCALE; }
    
    float Y_OFFSET[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd 
    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { Y_OFFSET[i] = state->y_offset * SCALE; }

    float NATURAL08[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd 
    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { NATURAL08[i] = (float)i; }

    void *pixels_void __aligned = NULL;
    int pitch = 0;

    if (flags_objs->use_graphics)
    {
        SDL_ERROR_HANDLE_(SDL_LockTexture(pixels_texture, NULL, &pixels_void, &pitch));
    }

    Uint32* pixels __aligned = (Uint32*)pixels_void;

    
    for (size_t repeat = 0; repeat < REP_CNT; ++repeat)
    {

// #ifdef COMPILE_OPTIMIZED
// #pragma omp parallel for collapse(1) schedule(guided)
// #endif /*COMPILE_OPTIMIZED*/

        for (size_t y_screen = 0; y_screen < SCREEN_HEIGHT; ++y_screen)
        {
            float y01[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd 
            for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y01[i] = (float)y_screen * SCALE; }
#pragma omp simd
            for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y01[i] -= Y_OFFSET[i]; }

            float y02[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
            for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y02[i] = y01[i]; }
            float y03[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
            for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y03[i] = y01[i]; }
            float y04[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
            for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y04[i] = y01[i]; }
    

            for (size_t x_screen = 0; x_screen < SCREEN_WIDTH; x_screen += SIMD_OBJS_CNT*UNROLL_CNT)
            {
                float x01[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd 
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x01[i] = (float)x_screen + 8*(1 - 1); }
                float x02[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd 
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x02[i] = (float)x_screen + 8*(2 - 1); }
                float x03[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd 
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x03[i] = (float)x_screen + 8*(3 - 1); }
                float x04[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd 
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x04[i] = (float)x_screen + 8*(4 - 1); }
                
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x01[i] += NATURAL08[i]; }
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x02[i] += NATURAL08[i]; }
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x03[i] += NATURAL08[i]; }
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x04[i] += NATURAL08[i]; }

#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x01[i] *= SCALE_VEC[i]; }
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x02[i] *= SCALE_VEC[i]; }
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x03[i] *= SCALE_VEC[i]; }
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x04[i] *= SCALE_VEC[i]; }

#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x01[i] -= X_OFFSET[i]; }
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x02[i] -= X_OFFSET[i]; }
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x03[i] -= X_OFFSET[i]; }
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x04[i] -= X_OFFSET[i]; }    

                volatile float iter1[SIMD_OBJS_CNT] __aligned = {0, 0, 0, 0, 0, 0, 0, 0};
                volatile float iter2[SIMD_OBJS_CNT] __aligned = {0, 0, 0, 0, 0, 0, 0, 0};
                volatile float iter3[SIMD_OBJS_CNT] __aligned = {0, 0, 0, 0, 0, 0, 0, 0};
                volatile float iter4[SIMD_OBJS_CNT] __aligned = {0, 0, 0, 0, 0, 0, 0, 0};  


                float x1[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x1[i] = x01[i]; } 
                float x2[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x2[i] = x02[i]; } 
                float x3[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x3[i] = x03[i]; } 
                float x4[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x4[i] = x04[i]; } 


                float y1[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y1[i] = y01[i]; } 
                float y2[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y2[i] = y02[i]; } 
                float y3[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y3[i] = y03[i]; } 
                float y4[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y4[i] = y04[i]; } 


                for (size_t iter_cnt = 0; iter_cnt < ITERS_CNT; ++iter_cnt)
                {
                    float xx1[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { xx1[i] = x1[i] * x1[i]; } 
                    float xx2[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { xx2[i] = x2[i] * x2[i]; } 
                    float xx3[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { xx3[i] = x3[i] * x3[i]; } 
                    float xx4[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { xx4[i] = x4[i] * x4[i]; } 
                    

                    float yy1[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { yy1[i] = y1[i] * y1[i]; }
                    float yy2[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { yy2[i] = y2[i] * y2[i]; }
                    float yy3[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { yy3[i] = y3[i] * y3[i]; }
                    float yy4[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { yy4[i] = y4[i] * y4[i]; }


                    float xy1[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { xy1[i] = x1[i] * y1[i]; }
                    float xy2[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { xy2[i] = x2[i] * y2[i]; }
                    float xy3[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { xy3[i] = x3[i] * y3[i]; }
                    float xy4[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { xy4[i] = x4[i] * y4[i]; }


                    float sum21[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { sum21[i] = xx1[i] + yy1[i]; }
                    float sum22[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { sum22[i] = xx2[i] + yy2[i]; }
                    float sum23[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { sum23[i] = xx3[i] + yy3[i]; }
                    float sum24[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { sum24[i] = xx4[i] + yy4[i]; }
                    

                    float cmp1[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { cmp1[i] = (sum21[i] <= R_CIRCLE_INF2_VEC[i] ? -1.f : 0.f); }
                    float cmp2[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { cmp2[i] = (sum22[i] <= R_CIRCLE_INF2_VEC[i] ? -1.f : 0.f); }
                    float cmp3[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { cmp3[i] = (sum23[i] <= R_CIRCLE_INF2_VEC[i] ? -1.f : 0.f); }
                    float cmp4[SIMD_OBJS_CNT] __aligned = {}; 
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { cmp4[i] = (sum24[i] <= R_CIRCLE_INF2_VEC[i] ? -1.f : 0.f); }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
                    uint32_t cmp_acc1 = 0; 
#pragma omp simd reduction(|:cmp_acc1)
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { cmp_acc1 |= *(uint32_t*)&cmp1[i]; }
                    int cmp_rez1 = (cmp_acc1 == 0); 
                    uint32_t cmp_acc2 = 0;
#pragma omp simd reduction(|:cmp_acc2)
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { cmp_acc2 |= *(uint32_t*)&cmp2[i]; }
                    int cmp_rez2 = (cmp_acc2 == 0); 
                    uint32_t cmp_acc3 = 0; 
#pragma omp simd reduction(|:cmp_acc3)
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { cmp_acc3 |= *(uint32_t*)&cmp3[i]; }
                    int cmp_rez3 = (cmp_acc3 == 0);
                    uint32_t cmp_acc4 = 0; 
#pragma omp simd reduction(|:cmp_acc4)
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { cmp_acc4 |= *(uint32_t*)&cmp4[i]; }
                    int cmp_rez4 = (cmp_acc4 == 0);
#pragma GCC diagnostic pop

                    if (cmp_rez1 && cmp_rez2 && cmp_rez3 && cmp_rez4) 
                        break;
                    
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter1[i] += (cmp1[i] != 0.0f) ? 1.0f : 0.0f; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter2[i] += (cmp2[i] != 0.0f) ? 1.0f : 0.0f; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter3[i] += (cmp3[i] != 0.0f) ? 1.0f : 0.0f; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter4[i] += (cmp4[i] != 0.0f) ? 1.0f : 0.0f; }

#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x1[i] = (xx1[i] - yy1[i]) + x01[i]; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x2[i] = (xx2[i] - yy2[i]) + x02[i]; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x3[i] = (xx3[i] - yy3[i]) + x03[i]; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { x4[i] = (xx4[i] - yy4[i]) + x04[i]; }

#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y1[i] = xy1[i] * 2.0f + y01[i]; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y2[i] = xy2[i] * 2.0f + y02[i]; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y3[i] = xy3[i] * 2.0f + y03[i]; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { y4[i] = xy4[i] * 2.0f + y04[i]; }

                }

                if (flags_objs->use_graphics)
                {
                    float iter[UNROLL_CNT*SIMD_OBJS_CNT] __aligned = {};
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter[i + SIMD_OBJS_CNT*(1-1)] = iter1[i]; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter[i + SIMD_OBJS_CNT*(2-1)] = iter2[i]; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter[i + SIMD_OBJS_CNT*(3-1)] = iter3[i]; }
#pragma omp simd
                    for (size_t i = 0; i < SIMD_OBJS_CNT; ++i) { iter[i + SIMD_OBJS_CNT*(4-1)] = iter4[i]; }

                    for (size_t i = 0; i < SIMD_OBJS_CNT*UNROLL_CNT; ++i)
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

#endif /*X86*/

#endif /*__AVX2__*/