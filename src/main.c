#include <stdio.h>
#include <locale.h>
#include <SDL2/SDL.h>

#include "utils/utils.h"
#include "logger/liblogger.h"
#include "flags/flags.h"

int init_all(flags_objs_t* const flags_objs, const int argc, char* const * argv, 
             SDL_Window** window, SDL_Renderer** renderer);
int dtor_all(flags_objs_t* const flags_objs, SDL_Window** window, SDL_Renderer** renderer);

int main(const int argc, char* const argv[])
{
    flags_objs_t    flags_objs  = {};
    SDL_Window*     window      = NULL;
    SDL_Renderer*   renderer    = NULL;

    INT_ERROR_HANDLE(init_all(&flags_objs, argc, argv, &window, &renderer));

    SDL_Event event = {};
    SDL_bool quit = SDL_FALSE;
    while (!quit) 
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quit = SDL_TRUE;
            }
        }

        SDL_ERROR_HANDLE(SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255),
                                                          dtor_all(&flags_objs, &window, &renderer);
        );
        SDL_ERROR_HANDLE(SDL_RenderClear(renderer),
                                                          dtor_all(&flags_objs, &window, &renderer);
        );

        SDL_RenderPresent(renderer);
    }

    INT_ERROR_HANDLE(dtor_all(&flags_objs, &window, &renderer));

    return EXIT_SUCCESS;
}

int logger_init(char* const log_folder);
int sdl_init(SDL_Window** window, SDL_Renderer** renderer);

int init_all(flags_objs_t* const flags_objs, const int argc, char* const * argv, 
             SDL_Window** window, SDL_Renderer** renderer)
{
    lassert(argc, "");
    lassert(!is_invalid_ptr(argv), "");
    lassert(!is_invalid_ptr(window), "");
    lassert(!is_invalid_ptr(renderer), "");

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

    SDL_ERROR_HANDLE(sdl_init(window, renderer),
                                                          logger_dtor();flags_objs_dtor(flags_objs);
    );

    return EXIT_SUCCESS;
}

int dtor_all(flags_objs_t* const flags_objs, SDL_Window** window, SDL_Renderer** renderer)
{
    LOGG_ERROR_HANDLE(                                                               logger_dtor());
    FLAGS_ERROR_HANDLE(                                                flags_objs_dtor(flags_objs));
                               SDL_DestroyRenderer(*renderer);SDL_DestroyWindow(*window);SDL_Quit();
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

int sdl_init(SDL_Window** window, SDL_Renderer** renderer)
{
    lassert(!is_invalid_ptr(window), "");
    lassert(!is_invalid_ptr(renderer), "");

    SDL_ERROR_HANDLE(SDL_Init(SDL_INIT_VIDEO));

    *window = SDL_CreateWindow(
        "Masturbator 2000", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        SCREEN_WIDTH, 
        SCREEN_HEIGHT, 
        SDL_WINDOW_SHOWN
    );

    if (!*window)
    {
        printf("Can`t SDL_CreateWindow. Error: %s\n", SDL_GetError());
                                                                                         SDL_Quit();
        return EXIT_FAILURE;
    }

    *renderer = SDL_CreateRenderer(
        *window, 
        -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!*renderer)
    {
        printf("SCan`t SDL_CreateRenderer. Error: %s\n", SDL_GetError());
                                                              SDL_DestroyWindow(*window);SDL_Quit();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}