#include <string.h>
#include <getopt.h>
#include <stdbool.h>

#include "flags/flags.h"
#include "logger/liblogger.h"
#include "utils/utils.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* flags_strerror(const enum FlagsError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(FLAGS_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(FLAGS_ERROR_FAILURE);
        default:
            return "UNKNOWN_FLAGS_ERROR";
    }
    return "UNKNOWN_FLAGS_ERROR";
}
#undef CASE_ENUM_TO_STRING_


enum FlagsError flags_objs_ctor(flags_objs_t* const flags_objs)
{
    lassert(!is_invalid_ptr(flags_objs), "");

    if (!strncpy(flags_objs->log_folder, "./log/", FILENAME_MAX))
    {
        perror("Can't strncpy flags_objs->log_folder");
        return FLAGS_ERROR_SUCCESS;
    }

    if (!strncpy(flags_objs->output_filename, "./assets/output.txt", FILENAME_MAX))
    {
        perror("Can't strncpy flags_objs->input_file");
        return FLAGS_ERROR_SUCCESS;
    }

    if (!strncpy(flags_objs->font_filename, "./assets/fonts/Montserrat-SemiBold.ttf", FILENAME_MAX))
    {
        perror("Can't strncpy flags_objs->input_file");
        return FLAGS_ERROR_SUCCESS;
    }


    flags_objs->input_file          = NULL;

    flags_objs->screen_width        = DEFAULT_SCREEN_WIDTH;
    flags_objs->screen_height       = DEFAULT_SCREEN_HEIGHT;
    flags_objs->screen_x_offset     = CENTERED_WINDOW_OPT;
    flags_objs->screen_y_offset     = CENTERED_WINDOW_OPT;

    flags_objs->use_graphics        = false;

    flags_objs->rep_calc_frame_cnt  = 1;
    flags_objs->frame_calc_cnt      = 0;

    return FLAGS_ERROR_SUCCESS;
}

enum FlagsError flags_objs_dtor (flags_objs_t* const flags_objs)
{
    lassert(!is_invalid_ptr(flags_objs), "");

    if (flags_objs->input_file && fclose(flags_objs->input_file))
    {
        perror("Can't fclose input file");
        return FLAGS_ERROR_FAILURE;
    }

    return FLAGS_ERROR_SUCCESS;
}


enum FlagsError flags_processing(flags_objs_t* const flags_objs, 
                                 const int argc, char* const argv[])
{
    lassert(!is_invalid_ptr(flags_objs), "");
    lassert(!is_invalid_ptr(argv), "");
    lassert(argc, "");

    int getopt_rez = 0;
    while ((getopt_rez = getopt(argc, argv, "l:o:w:h:x:y:s:r:f:c:g")) != -1)
    {
        switch (getopt_rez)
        {
            case 'l':
            {
                if (!strncpy(flags_objs->log_folder, optarg, FILENAME_MAX))
                {
                    perror("Can't strncpy flags_objs->log_folder");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }

            case 'o':
            {
                if (!strncpy(flags_objs->output_filename, optarg, FILENAME_MAX))
                {
                    perror("Can't strncpy flags_objs->output_filename");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }

            case 'w':
            {
                if ((flags_objs->screen_width = atoi(optarg)) == 0)
                {
                    perror("Can't atoi screen width");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }

            case 'h':
            {
                if ((flags_objs->screen_height = atoi(optarg)) == 0)
                {
                    perror("Can't atoi screen height");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }

            case 'x':
            {
                flags_objs->screen_x_offset = atoi(optarg);

                break;
            }

            case 'y':
            {
                flags_objs->screen_y_offset = atoi(optarg);

                break;
            }

            case 's':
            {
                if (sscanf(optarg, "%dx%dx%dx%d",
                       &flags_objs->screen_width,    &flags_objs->screen_height,
                       &flags_objs->screen_x_offset, &flags_objs->screen_y_offset)
                    != 4)
                {
                    perror("Can't sscanf screen sizes and offsets");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }

            case 'g':
            {
                flags_objs->use_graphics = true;

                break;
            }

            case 'r':
            {
                if ((flags_objs->rep_calc_frame_cnt = (size_t)atoll(optarg)) == 0)
                {
                    perror("Can't atoi count repeat caluclation one frame");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }

            case 'f':
            {
                if (!strncpy(flags_objs->font_filename, optarg, FILENAME_MAX))
                {
                    perror("Can't strncpy flags_objs->font_filename");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }

            case 'c':
            {
                flags_objs->frame_calc_cnt = (size_t)atoll(optarg);

                break;
            }

            default:
            {
                fprintf(stderr, "Getopt error - d: %d, c: %c\n", getopt_rez, (char)getopt_rez);
                return FLAGS_ERROR_FAILURE;
            }
        }
    }

    if (flags_objs->use_graphics && flags_objs->frame_calc_cnt != 0)
    {
        fprintf(stderr, "Invalid flags combintaions\n");
        return FLAGS_ERROR_FAILURE;
    }

    return FLAGS_ERROR_SUCCESS;
}