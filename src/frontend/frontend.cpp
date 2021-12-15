#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>

#include "parser/parser.h"
#include "lexer/lexer.h"
#include "frontend.h"
#include "../tree/Tree.h"
#include "../dumpsystem/dumpsystem.h"
#include "jumps.h"

static int get_file_sz_(const char filename[], size_t* sz)
{
    struct stat buff = {};
    if(stat(filename, &buff) == -1)
        return -1;
    
    *sz = buff.st_size;
    
    return 0;
}

int main()
{
    tree_dump_init(dumpsystem_get_stream(log));
    token_dump_init(dumpsystem_get_stream(log));

    char ifilename[] = "code.txt";
    char ofilename[] = "tree.txt";
    char* data = nullptr;

    FILE* istream = nullptr;
    FILE* ostream = nullptr;

    Tree tree = {};
    Token_array tok_arr = {};
    Token_nametable tok_table = {};

    size_t file_sz = 0;
    lexer_err lexer_error = LEXER_NOERR;

TRY__
    ASSERT$(get_file_sz_(ifilename, &file_sz) != -1, 
                                               FRONTEND_READ_FAIL, FAIL__; );

    data = (char*) calloc(file_sz, sizeof(char));
    ASSERT$(data,                              FRONTEND_BAD_ALLOC, FAIL__; );

    istream = fopen(ifilename, "r");
    ASSERT$(istream,                           FRONTEND_READ_FAIL, FAIL__; );

    file_sz = fread(data, sizeof(char), file_sz, istream);
    ASSERT$(!ferror(istream),                  FRONTEND_READ_FAIL, FAIL__; );

    fclose(istream);
    istream = nullptr;

    data = (char*) realloc(data, file_sz * sizeof(char));
    ASSERT$(data,                              FRONTEND_BAD_ALLOC, FAIL__; );

    lexer_error = lexer(&tok_arr, &tok_table, data, file_sz);
    token_nametable_dump(&tok_table);
    token_array_dump(&tok_arr);

    ASSERT$(!lexer_error,                      FRONTEND_LEXER_FAIL, FAIL__; );

    ASSERT$(!parse(&tree, &tok_arr),           FRONTEND_PARSER_FAIL, FAIL__; );

    ostream = fopen(ofilename, "w");
    ASSERT$(ostream,                           FRONTEND_WRITE_FAIL, FAIL__; );

    tree_write(&tree, ostream);
    ASSERT$(!ferror(ostream),                  FRONTEND_WRITE_FAIL, FAIL__; );

    fclose(ostream);
    ostream = nullptr;

CATCH__
    if(istream)
        fclose(istream);

    if(ostream)
        fclose(ostream);
    
    tree_dstr(&tree);
    token_array_dstr(&tok_arr);
    token_nametable_dstr(&tok_table);

FINALLY__
    return 0;

ENDTRY__
}
