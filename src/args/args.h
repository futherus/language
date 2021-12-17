#ifndef ARGS_H
#define ARGS_H

#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 1
#endif
#include <stdio.h>

enum args_msg
{
    ARGS_NOMSG           = 0,  /// no message
    ARGS_HLP             = 1,  /// reference
    ARGS_HLP_NOTE        = 2,  /// reference with note
    ARGS_NO_OPT          = 3,  /// error: no options
    ARGS_OPT_OVRWRT      = 4,  /// options overwrite
    ARGS_NO_IFL_NAME     = 5,  /// no input file name
    ARGS_NO_OFL_NAME     = 6,  /// no output file name
    ARGS_FLNAME_OVRFLW   = 8,  /// filename overflow
    ARGS_BAD_CMD         = 9,  /// bad input for command
    ARGS_UNEXPCTD_ERR    = 10, ///     
};

const char HELP[]              = "-h, --help             reference\n"
                                 "--src <filename>       input file\n"
                                 "--dst <filename>       output file\n";

const char NOTE[]              = "(program ignores other options if -h entered)\n";
const char NO_OPTIONS[]        = "\x1b[31;1mError:\x1b[0m enter options (-h to open reference)\n";
const char OPTION_OVERWRITE[]  = "\x1b[31;1mError:\x1b[0m option overwrite\n";
const char NO_IFILE_NAME[]     = "\x1b[31;1mError:\x1b[0m enter source file name\n";
const char NO_OFILE_NAME[]     = "\x1b[31;1mError:\x1b[0m enter destination file name\n";
const char FILENAME_OVERFLOW[] = "\x1b[31;1mError:\x1b[0m filename is too long\n";
const char BAD_COMMAND[]       = "\x1b[31;1mError:\x1b[0m bad command\n";
const char UNEXPECTED_ERROR[]  = "\x1b[31;1mUNEXPECTED ERROR\x1b[0m\n";

/** \brief Prints message corresponding to param

    \param [out] ostream FILE* pointer to output file
    \param [in]  param   args_msg parameter, sets message
*/
void response_args(args_msg param, FILE* const ostream = stderr);

/** \brief Processes command line arguments and sets according parameters

    \param [in]  argc         Number of arguments
    \param [in]  argv         Array of arguments
    \param [out] infile_name  Name of input file
    \param [out] outfile_name Name of output file

    \return ARGS_NOMSG, args_msg with error otherwise
*/
args_msg process_args(int argc, char* argv[], char infile_name[], char outfile_name[]);

#endif // ARGS_H
