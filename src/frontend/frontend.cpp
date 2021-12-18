#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>

#include "parser/parser.h"
#include "lexer/lexer.h"
#include "frontend.h"
#include "../tree/Tree.h"
#include "../dumpsystem/dumpsystem.h"
#include "../jumps.h"
#include "../args/args.h"

static int get_file_sz_(const char filename[], size_t* sz)
{
    struct stat buff = {};
    if(stat(filename, &buff) == -1)
        return -1;
    
    *sz = buff.st_size;
    
    return 0;
}

int main(int argc, char* argv[])
{
    tree_dump_init(dumpsystem_get_stream(frontend_log));
    token_dump_init(dumpsystem_get_stream(frontend_log));

    char  infile_name[FILENAME_MAX]  = "";
    char  outfile_name[FILENAME_MAX] = "";
    char* data = nullptr;

    FILE* istream = nullptr;
    FILE* ostream = nullptr;

    Tree tree = {};
    Token_array tok_arr = {};
    Token_nametable tok_table = {};

    size_t file_sz = 0;
    lexer_err lexer_error = LEXER_NOERR;

    args_msg msg = process_args(argc, argv, infile_name, outfile_name);
    if(msg)
    {
        response_args(msg);
        return FRONTEND_ARGS_ERROR;
    }

TRY__
    CHECK__(get_file_sz_(infile_name, &file_sz) == -1,
                                               FRONTEND_INFILE_FAIL);

    data = (char*) calloc(file_sz, sizeof(char));
    CHECK__(!data,                             FRONTEND_BAD_ALLOC);

    istream = fopen(infile_name, "r");
    CHECK__(!istream,                          FRONTEND_INFILE_FAIL);

    file_sz = fread(data, sizeof(char), file_sz, istream);
    CHECK__(ferror(istream),                   FRONTEND_READ_FAIL);

    fclose(istream);
    istream = nullptr;

    data = (char*) realloc(data, file_sz * sizeof(char));
    CHECK__(!data,                             FRONTEND_BAD_ALLOC);

    lexer_error = lexer(&tok_arr, &tok_table, data, file_sz);

    free(data);
    data = nullptr;

    token_nametable_dump(&tok_table);
    token_array_dump(&tok_arr);

    CHECK__(lexer_error,                       FRONTEND_LEXER_FAIL);

    CHECK__(parse(&tree, &tok_arr),            FRONTEND_PARSER_FAIL);

    ostream = fopen(outfile_name, "w");
    CHECK__(!ostream,                          FRONTEND_OUTFILE_FAIL);

    tree_write(&tree, ostream);
    CHECK__(ferror(ostream),                   FRONTEND_WRITE_FAIL);

    fclose(ostream);
    ostream = nullptr;

CATCH__
    if(istream)
        fclose(istream);

    if(ostream)
        fclose(ostream);
    
    free(data);

FINALLY__
    tree_dstr(&tree);
    token_array_dstr(&tok_arr);
    token_nametable_dstr(&tok_table);

    return ERROR__;

ENDTRY__
}
