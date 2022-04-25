#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "args.h"

args_msg process_args(int argc, char* argv[], char infile_name[], char outfile_name[])
{
    assert(argv);

    if(argc < 2)
        return ARGS_NO_OPT;

    for(int iter = 1; iter < argc; iter++)
    {
        if(strcmp(argv[iter], "-h") == 0 || strcmp(argv[iter], "--help") == 0)
        {
            if(argc > 2)
                return ARGS_HLP_NOTE;
            else
                return ARGS_HLP;
        }
        else if(strcmp(argv[iter], "--src") == 0)
        {
            if(infile_name[0] != 0)
                return ARGS_OPT_OVRWRT;

            iter++;
            if(iter >= argc)
                return ARGS_NO_IFL_NAME;

            if(memccpy(infile_name, argv[iter], '\0', FILENAME_MAX) == nullptr)
                return ARGS_FLNAME_OVRFLW; //LONG_FILENAME
        }
        else if(outfile_name && strcmp(argv[iter], "--dst") == 0)
        {
            if(outfile_name[0] != 0)
                return ARGS_OPT_OVRWRT;
            
            iter++;
            if(iter >= argc)
                return ARGS_NO_OFL_NAME;

            if(memccpy(outfile_name, argv[iter], '\0', FILENAME_MAX) == nullptr)
                return ARGS_FLNAME_OVRFLW; //LONG_FILENAME
        }
        else
        {
            return ARGS_BAD_CMD;
        }
    }
    
    if(infile_name[0] == 0)
        return ARGS_NO_IFL_NAME;
    if(outfile_name[0] == 0)
        return ARGS_NO_OFL_NAME;

    return ARGS_NOMSG;
}

void response_args(args_msg param, FILE* const ostream)
{
    assert(ostream);

    switch(param)
    {
        case ARGS_HLP:
        {
            fprintf(ostream, HELP);
            break;
        }
        case ARGS_HLP_NOTE:
        {
            fprintf(ostream, "%s\n%s", HELP, NOTE);
            break;
        }
        case ARGS_NO_OPT:
        {
            fprintf(ostream, NO_OPTIONS);
            break;
        }
        case ARGS_OPT_OVRWRT:
        {
            fprintf(ostream, OPTION_OVERWRITE);
            break;
        }
        case ARGS_NO_IFL_NAME:
        {
            fprintf(ostream, NO_IFILE_NAME);
            break;
        }
        case ARGS_NO_OFL_NAME:
        {
            fprintf(ostream, NO_OFILE_NAME);
            break;
        }
        case ARGS_FLNAME_OVRFLW:
        {
            fprintf(ostream, FILENAME_OVERFLOW);
            break;
        }
        case ARGS_BAD_CMD:
        {
            fprintf(ostream, BAD_COMMAND);
            break;
        }
        case ARGS_UNEXPCTD_ERR:
        {
            fprintf(ostream, UNEXPECTED_ERROR);
            break;
        }
        case ARGS_NOMSG:
        {
            break;
        }
        default:
        {
            assert(0);
        }
    }
}
