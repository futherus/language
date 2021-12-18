#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../tree/Tree.h"
#include "backend.h"
#include "generator.h"
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
    tree_dump_init(dumpsystem_get_stream(backend_log));
    token_dump_init(dumpsystem_get_stream(backend_log));

    char  infile_name[FILENAME_MAX]  = "";
    char  outfile_name[FILENAME_MAX] = "";
    char* data = nullptr;

    FILE* istream = nullptr;
    FILE* ostream = nullptr;

    Tree tree = {};
    Token_nametable tok_table = {};

    size_t file_sz = 0;
    args_msg msg = ARGS_NOMSG;

    msg = process_args(argc, argv, infile_name, outfile_name);
    if(msg)
    {
        response_args(msg);
        LOG$("Error: wrong arguments entered\n");
        return 0;
    }

TRY__
    CHECK__(get_file_sz_(infile_name, &file_sz) == -1,
                                                         BACKEND_INFILE_FAIL);

    data = (char*) calloc(file_sz, sizeof(char));
    CHECK__(!data,                                       BACKEND_BAD_ALLOC);

    istream = fopen(infile_name, "r");
    CHECK__(!istream,                                    BACKEND_INFILE_FAIL);

    file_sz = fread(data, sizeof(char), file_sz, istream);
    CHECK__(ferror(istream),                             BACKEND_READ_FAIL);

    fclose(istream);
    istream = nullptr;

    data = (char*) realloc(data, file_sz * sizeof(char));
    CHECK__(!data,                                       BACKEND_BAD_ALLOC);

    CHECK__(tree_read(&tree, &tok_table, data, file_sz), BACKEND_FORMAT_ERROR);
    tree_dump(&tree, "Dump");
    token_nametable_dump(&tok_table);

    ostream = fopen(outfile_name, "w");
    CHECK__(!ostream,                                    BACKEND_INFILE_FAIL);

    CHECK__(generator(&tree, ostream),                   BACKEND_GENERATOR_FAIL);

    fclose(ostream);
    ostream = nullptr;

    free(data);
    data = nullptr;

CATCH__
    if(istream)
        fclose(istream);
    
    if(ostream)
        fclose(ostream);
    
FINALLY__
    free(data);

    tree_dstr(&tree);
    token_nametable_dstr(&tok_table);

    return ERROR__;

ENDTRY__
}