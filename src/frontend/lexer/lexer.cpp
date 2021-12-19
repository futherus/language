#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "../../dumpsystem/dumpsystem.h"

static void clean_whitespaces_(const char data[], ptrdiff_t* pos)
{
    assert(data && pos);

    while(isspace(data[*pos]))
        (*pos)++;
}

static bool get_number(const char data[], double* num, int* n_read)
{
    if(isdigit(*data))
    {
        sscanf(data, "%lg%n", num, n_read);
        return true;
    }

    return false;
}

static void wordlen_(const char data[], int* n_read)
{
    assert(data && n_read);

    while((!ispunct(data[*n_read]) || data[*n_read] == '_') && !isspace(data[*n_read]))
        (*n_read)++;
}

#define lexer_err(MSG_, PTR_)                                                           \
do                                                                                      \
{                                                                                       \
    char tmp_buffer_[128] = "";                                                         \
    strncpy(tmp_buffer_, (PTR_), 126);                                                  \
    tmp_buffer_[127] = '\0';                                                            \
                                                                                        \
    fprintf(stderr, "\x1b[31mLexer error:\x1b[0m %s : %s\n", (MSG_), tmp_buffer_);      \
    FILE* stream_ = dumpsystem_get_opened_stream();                                     \
                                                                                        \
    if(stream_)                                                                         \
    {                                                                                   \
        fprintf(stream_, "<span class = \"error\">Lexer error: %s : %s\n</span>"        \
                         "\t\t\t\tat %s:%d:%s\n",                                       \
                         (MSG_), tmp_buffer_,                                           \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__);                      \
                                                                                        \
        return LEXER_LEXICAL_ERROR;                                                     \
    }                                                                                   \
} while(0)                                                                              \

#define DEF_KEY(NAME, STD_NAME, MANGLE)                         \
    if(n_read == 0 || n_read == sizeof(NAME) - 1)               \
    {                                                           \
        if(strncmp(data + pos, (NAME), sizeof(NAME) - 1) == 0)  \
        {                                                       \
            tmp.type = TYPE_KEYWORD;                            \
            tmp.val.key = TOK_##MANGLE;                         \
            pos += sizeof(NAME) - 1;                            \
            PASS$(!token_array_add(tok_arr, &tmp), return LEXER_BAD_ALLOC; ); \
            continue;                                           \
        }                                                       \
    }                                                           \

#define DEF_OP(NAME, STD_NAME, MANGLE)                          \
    if(n_read == 0 || n_read == sizeof(NAME) - 1)               \
    {                                                           \
        if(strncmp(data + pos, (NAME), sizeof(NAME) - 1) == 0)  \
        {                                                       \
            tmp.type = TYPE_OP;                                 \
            tmp.val.op = TOK_##MANGLE;                          \
            pos += sizeof(NAME) - 1;                            \
            PASS$(!token_array_add(tok_arr, &tmp), return LEXER_BAD_ALLOC; ); \
            continue;                                           \
        }                                                       \
    }                                                           \

#define DEF_EMB(NAME, STD_NAME, MANGLE)                         \
    if(n_read == 0 || n_read == sizeof(NAME) - 1)               \
    {                                                           \
        if(strncmp(data + pos, (NAME), sizeof(NAME) - 1) == 0)  \
        {                                                       \
            tmp.type = TYPE_EMBED;                              \
            tmp.val.emb = TOK_##MANGLE;                         \
            pos += sizeof(NAME) - 1;                            \
            PASS$(!token_array_add(tok_arr, &tmp), return LEXER_BAD_ALLOC; ); \
            continue;                                           \
        }                                                       \
    }                                                           \

lexer_err lexer(Token_array* tok_arr, Token_nametable* tok_table, const char data[], ptrdiff_t data_sz)
{
    assert(tok_arr && tok_table && data);
    ASSERT_RET$(data_sz > 0, LEXER_EMPTY_DATA);
    
    ptrdiff_t pos = 0;
    Token tmp = {};

    while(true)
    {
        clean_whitespaces_(data, &pos);   
        if(pos == data_sz)
            break;
        
        int n_read = 0;
        tmp = {};

        if(get_number(data + pos, &tmp.val.num, &n_read))
        {
            tmp.type = TYPE_NUMBER;
            pos += n_read;
            PASS$(!token_array_add(tok_arr, &tmp), return LEXER_BAD_ALLOC; );

            continue;
        }
        wordlen_(data + pos, &n_read);

        if(data[pos] == '>')
        {
            ptrdiff_t comment_start = pos;

            while(data[pos] != '<' && data[pos] != '\0')
                pos++;
            
            if(data[pos] == '\0')
                lexer_err("Unterminated comment", data + comment_start);
            
            pos++;
            continue;
        }

        if(data[pos] == '<')
            lexer_err("Comment termination without comment start", data + pos);

        #include "../../reserved_operators.inc"
        #include "../../reserved_keywords.inc"
        #include "../../reserved_embedded.inc"

        if(!n_read)
            lexer_err("Unknown symbol", data + pos);

        if(strncmp("пачатак", data + pos, n_read) == 0)
            PASS$(!token_nametable_add(tok_table, &tmp.val.name, "main", sizeof("main") - 1), return LEXER_BAD_ALLOC; );
        else
            PASS$(!token_nametable_add(tok_table, &tmp.val.name, data + pos, n_read), return LEXER_BAD_ALLOC; );

        tmp.type = TYPE_ID;

        PASS$(!token_array_add(tok_arr, &tmp), return LEXER_BAD_ALLOC; );
        pos += n_read;
    }

    tok_arr->eof = tok_arr->size;
    tmp.type = TYPE_EOF;
    PASS$(!token_array_add(tok_arr, &tmp), return LEXER_BAD_ALLOC; );

    return LEXER_NOERR;
}

#undef DEF_OP
#undef DEF_KEY
#undef DEF_EMB

void consume(Token* tok, Token_array* tok_arr)
{
    assert(tok && tok_arr);
    assert(tok_arr->pos <= tok_arr->eof);

    *tok = tok_arr->data[tok_arr->pos];
    tok_arr->pos++;
}

void peek(Token* tok, ptrdiff_t offset, Token_array* tok_arr)
{
    assert(tok && tok_arr);

    if(tok_arr->pos + offset < 0 || tok_arr->pos + offset > tok_arr->eof)
    {
        *tok = tok_arr->data[tok_arr->eof];
        
        return;
    }

    *tok = tok_arr->data[tok_arr->pos + offset];
}
