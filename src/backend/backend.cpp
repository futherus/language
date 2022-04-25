#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "backend.h"
#include "generator.h"
#include "../common/dumpsystem.h"
#include "../common/jumps.h"
#include "../common/args.h"

static int get_file_sz_(const char filename[], size_t* sz)
{
    struct stat buff = {};
    if(stat(filename, &buff) == -1)
        return -1;
    
    *sz = (size_t) buff.st_size;
    
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
    ASSERT$(get_file_sz_(infile_name, &file_sz) != -1,
                                                            BACKEND_INFILE_FAIL,    FAIL__);

    data = (char*) calloc(file_sz + 1, sizeof(char));
    ASSERT$(data,                                           BACKEND_BAD_ALLOC,      FAIL__);

    istream = fopen(infile_name, "r");
    ASSERT$(istream,                                        BACKEND_INFILE_FAIL,    FAIL__);

    file_sz = fread(data, sizeof(char), file_sz, istream);
    ASSERT$(!ferror(istream),                               BACKEND_READ_FAIL,      FAIL__);
    fclose(istream);
    istream = nullptr;

    data = (char*) realloc(data, (file_sz + 1) * sizeof(char));
    ASSERT$(data,                                           BACKEND_BAD_ALLOC,      FAIL__);
    data[file_sz] = 0; // null-termination of buffer

    ASSERT$(!tree_read(&tree, &tok_table, data, (ptrdiff_t) file_sz),   
                                                            BACKEND_FORMAT_ERROR,   FAIL__);
    ostream = fopen(outfile_name, "w");
    ASSERT$(ostream,                                        BACKEND_OUTFILE_FAIL,   FAIL__);

    ASSERT$(!generator(&tree, ostream),                     BACKEND_GENERATOR_FAIL, FAIL__);

    fclose(ostream);
    ostream = nullptr;

    free(data);
    data = nullptr;

CATCH__
    ERROR__ = 1;

    if(istream)
        fclose(istream);
    
    if(ostream)
        fclose(ostream);
    
    free(data);

FINALLY__

    tree_dstr(&tree);
    token_nametable_dstr(&tok_table);

    return ERROR__;

ENDTRY__
}
