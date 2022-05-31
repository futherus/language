#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "elf_backend.h"
#include "elf_generator.h"
#include "elf_wrap.h"
#include "../../include/logs/logs.h"
#include "../common/depend.h"
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
    logs_init      ("logs/backend_elf_log.html");
    tree_dump_init (logs_get());
    token_dump_init(logs_get());

    char  infile_name [FILENAME_MAX] = "";
    char  outfile_name[FILENAME_MAX] = "";
    char* depfile_name = nullptr;

    char* data    = nullptr;
    char* depdata = nullptr;
    size_t infile_sz  = 0;
    size_t depfile_sz = 0;

    FILE* istream = nullptr;
    FILE* ostream = nullptr;

    Binary          bin       = {};
    Tree            tree      = {};
    Dependencies    deps      = {};
    Token_nametable tok_table = {};
    
    args_msg msg = ARGS_NOMSG;

    msg = process_args(argc, argv, infile_name, outfile_name);
    if(msg)
    {
        response_args(msg);
        LOG$("Error: wrong arguments entered\n");
        return 0;
    }

TRY__
    ASSERT$(!dep_get_filename(&depfile_name, infile_name),
                                                            BACKEND_ELF_INFILE_FAIL,    FAIL__);
    ASSERT$(get_file_sz_(infile_name,  &infile_sz)  != -1,
                                                            BACKEND_ELF_INFILE_FAIL,    FAIL__);
    // FIXME assume that program has no dependencies if cannot find or open such file
    ASSERT$(get_file_sz_(depfile_name, &depfile_sz) != -1,
                                                            BACKEND_ELF_INFILE_FAIL,    FAIL__);

    depdata = (char*) calloc(depfile_sz + 1, sizeof(char));
    ASSERT$(depdata,                                        BACKEND_ELF_BAD_ALLOC,      FAIL__);

    istream = fopen(depfile_name, "r");
    ASSERT$(istream,                                        BACKEND_ELF_INFILE_FAIL,    FAIL__);

    depfile_sz = fread(depdata, sizeof(char), depfile_sz, istream);
    ASSERT$(!ferror(istream),                               BACKEND_ELF_READ_FAIL,      FAIL__);
    fclose(istream);
    istream = nullptr;

    ASSERT$(!dep_read(&deps, depdata, depfile_sz),          BACKEND_ELF_INFILE_FAIL,    FAIL__);

    data = (char*) calloc(infile_sz + 1, sizeof(char));
    ASSERT$(data,                                           BACKEND_ELF_BAD_ALLOC,      FAIL__);

    istream = fopen(infile_name, "r");
    ASSERT$(istream,                                        BACKEND_ELF_INFILE_FAIL,    FAIL__);

    infile_sz = fread(data, sizeof(char), infile_sz, istream);
    ASSERT$(!ferror(istream),                               BACKEND_ELF_READ_FAIL,      FAIL__);
    fclose(istream);
    istream = nullptr;

    data = (char*) realloc(data, (infile_sz + 1) * sizeof(char));
    ASSERT$(data,                                           BACKEND_ELF_BAD_ALLOC,      FAIL__);
    data[infile_sz] = 0; // null-termination of buffer

    ASSERT$(!tree_read(&tree, &tok_table, data, (ptrdiff_t) infile_sz),   
                                                            BACKEND_ELF_FORMAT_ERROR,   FAIL__);

    ASSERT$(!generator(&tree, &deps, &bin),                 BACKEND_ELF_GENERATOR_FAIL, FAIL__);
    
    ostream = fopen(outfile_name, "wb");
    ASSERT$(ostream,                                        BACKEND_ELF_OUTFILE_FAIL,   FAIL__);

    ASSERT$(!binary_write(ostream, &bin),                   BACKEND_ELF_OUTFILE_FAIL,   FAIL__);

    fclose(ostream);
    ostream = nullptr;

CATCH__
    ERROR__ = 1;

    if(istream)
        fclose(istream);
    
    if(ostream)
        fclose(ostream);
    
FINALLY__
    free(data);
    free(depdata);
    free(depfile_name);
    
    dep_dtor            (&deps);
    binary_dtor         (&bin);
    tree_dstr           (&tree);
    token_nametable_dstr(&tok_table);

    return ERROR__;

ENDTRY__
}
