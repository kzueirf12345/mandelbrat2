#include <stdio.h>
#include <locale.h>

#include <SDL2/SDL.h>

#include "utils/utils.h"
#include "logger/liblogger.h"
#include "flags/flags.h"
#include "mandelbrat2/mandelbrat2.h"
#include "sdl_objs/sdl_objs.h"
#include "time_checker/time_checker.h"

int init_all(flags_objs_t* const flags_objs, const int argc, char* const * argv, 
             sdl_objs_t* const sdl_objs,
             mandelbrat2_state_t* const state);
int dtor_all(flags_objs_t* const flags_objs, sdl_objs_t* const sdl_objs);

int main(const int argc, char* const argv[])
{
    flags_objs_t        flags_objs  = {};
    sdl_objs_t          sdl_objs    = {};
    mandelbrat2_state_t state       = {};

    INT_ERROR_HANDLE(init_all(&flags_objs, argc, argv, &sdl_objs, &state));

    SDL_Event event = {};
    SDL_bool quit = SDL_FALSE;
    size_t frame_cnt = 0;
    while (!quit) 
    {
        if (flags_objs.use_graphics)
        {
            SDL_OBJS_ERROR_HANDLE(sdl_handle_events(&event, &flags_objs, &state, &quit),
                                                                   dtor_all(&flags_objs, &sdl_objs);
            );

            SDL_ERROR_HANDLE(SDL_RenderClear(sdl_objs.renderer),
                                                                   dtor_all(&flags_objs, &sdl_objs);
            );
        }

        MANDELBRAT2_ERROR_HANDLE(print_frame(sdl_objs.pixels_texture, &state, &flags_objs),
                                                                   dtor_all(&flags_objs, &sdl_objs);
        );

        if (flags_objs.use_graphics)
        {
            SDL_ERROR_HANDLE(SDL_RenderCopy(sdl_objs.renderer, sdl_objs.pixels_texture, NULL, NULL),
                                                                   dtor_all(&flags_objs, &sdl_objs);
            );
        }

        TIME_CHECKER_ERROR_HANDLE(time_checker_update(&sdl_objs),
                                                                   dtor_all(&flags_objs, &sdl_objs);
        );

        if (flags_objs.use_graphics)
        {
            SDL_RenderPresent(sdl_objs.renderer);
        }

        ++frame_cnt;

        if (flags_objs.frame_calc_cnt != 0 && frame_cnt >= flags_objs.frame_calc_cnt)
        {
            quit = true;
        }
    }

    INT_ERROR_HANDLE(                                            dtor_all(&flags_objs, &sdl_objs););

    return EXIT_SUCCESS;
}

int logger_init(char* const log_folder);

int init_all(flags_objs_t* const flags_objs, const int argc, char* const * argv, 
             sdl_objs_t* const sdl_objs,
             mandelbrat2_state_t* const state)
{
    lassert(argc, "");
    lassert(!is_invalid_ptr(argv), "");
    lassert(!is_invalid_ptr(sdl_objs), "");
    lassert(!is_invalid_ptr(state), "");

    if (!setlocale(LC_ALL, "ru_RU.utf8"))
    {
        fprintf(stderr, "Can't setlocale\n");
        return EXIT_FAILURE;
    }

    FLAGS_ERROR_HANDLE(flags_objs_ctor (flags_objs));
    FLAGS_ERROR_HANDLE(flags_processing(flags_objs, argc, argv));

    if (logger_init(flags_objs->log_folder))
    {
        fprintf(stderr, "Can't logger init\n");
                                                                        flags_objs_dtor(flags_objs);
        return EXIT_FAILURE;
    }
    
    if (flags_objs->use_graphics)
    {
        SDL_OBJS_ERROR_HANDLE(sdl_objs_ctor(sdl_objs, flags_objs),
                                                                        flags_objs_dtor(flags_objs);
                                                                                      logger_dtor();
        );
    }
    TIME_CHECKER_ERROR_HANDLE(
        time_checker_ctor(FPS_FREQ_MS, flags_objs->use_graphics, flags_objs->output_filename), 
                                                                        sdl_objs_dtor(sdl_objs);
                                                                    flags_objs_dtor(flags_objs);
                                                                                  logger_dtor();
    );

    MANDELBRAT2_ERROR_HANDLE(mandelbrat2_state_ctor(state, flags_objs),
                                                                                 time_checker_dtor();
                                                                            sdl_objs_dtor(sdl_objs);
                                                                        flags_objs_dtor(flags_objs);
                                                                                      logger_dtor();
    );

    return EXIT_SUCCESS;
}

int dtor_all(flags_objs_t* const flags_objs, sdl_objs_t* const sdl_objs)
{
    if (flags_objs->use_graphics)
    {
                                                                            sdl_objs_dtor(sdl_objs);
    }
    TIME_CHECKER_ERROR_HANDLE(                                                 time_checker_dtor());

    LOGG_ERROR_HANDLE(                                                               logger_dtor());
    FLAGS_ERROR_HANDLE(                                                flags_objs_dtor(flags_objs));

    return EXIT_SUCCESS;
}

#define LOGOUT_FILENAME "logout.log"
#define   DUMB_FILENAME "dumb"
int logger_init(char* const log_folder)
{
    lassert(!is_invalid_ptr(log_folder), "");

    char logout_filename[FILENAME_MAX] = {};
    if (snprintf(logout_filename, FILENAME_MAX, "%s%s", log_folder, LOGOUT_FILENAME) <= 0)
    {
        perror("Can't snprintf logout_filename");
        return EXIT_FAILURE;
    }

    char dumb_filename[FILENAME_MAX] = {};
    if (snprintf(dumb_filename, FILENAME_MAX, "%s%s", log_folder, DUMB_FILENAME) <= 0)
    {
        perror("Can't snprintf dumb_filename");
        return EXIT_FAILURE;
    }

    LOGG_ERROR_HANDLE(logger_ctor());
    LOGG_ERROR_HANDLE(logger_set_level_details(LOG_LEVEL_DETAILS_ALL));
    LOGG_ERROR_HANDLE(logger_set_logout_file(logout_filename));
    
    return EXIT_SUCCESS;
}
#undef LOGOUT_FILENAME
#undef   DUMB_FILENAME