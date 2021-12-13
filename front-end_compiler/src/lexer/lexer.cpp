#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "../hash.h"
#include "lexer.h"
#include "../dumpsystem/dumpsystem.h"

static const ptrdiff_t TOK_ARR_MIN_CAP_     = 8;
static const ptrdiff_t TOK_ARR_CAP_MULTPLR_ = 2;

static struct Token_arr
{
    Token* data = nullptr;
    ptrdiff_t size = 0;
    ptrdiff_t cap  = 0;

    ptrdiff_t pos  = 0;
    ptrdiff_t eof  = 0;
} TOKEN_ARRAY_;

static lexer_err resize_(Token_arr* tok_arr)
{
    assert(tok_arr);

    ptrdiff_t new_cap = 0;
    if(tok_arr->cap == 0)
        new_cap = TOK_ARR_MIN_CAP_;
    else
        new_cap = tok_arr->cap * TOK_ARR_CAP_MULTPLR_;

    Token* new_data = (Token*) realloc(tok_arr->data, new_cap * sizeof(Token));
    ASSERT_RET$(new_data, LEXER_BAD_ALLOC);

    tok_arr->data = new_data;
    tok_arr->cap  = new_cap;

    return LEXER_NOERR;
}

static lexer_err add_token_(const Token* token)
{
    assert(token);

    if(TOKEN_ARRAY_.cap == TOKEN_ARRAY_.size)
        PASS$(!resize_(&TOKEN_ARRAY_), return LEXER_BAD_ALLOC; );
    
    *(TOKEN_ARRAY_.data + TOKEN_ARRAY_.size) = *token;
    TOKEN_ARRAY_.size++;

    return LEXER_NOERR;
}

static void clean_whitespaces_(char* txt, ptrdiff_t* pos)
{
    assert(txt && pos);

    while(isspace(txt[*pos]))
        (*pos)++;
}

static bool get_number(char* txt, double* num, int* n_read)
{
    if(isdigit(*txt))
    {
        sscanf(txt, "%lg%n", num, n_read);
        return true;
    }

    return false;
}

static void wordlen_(char* txt, int* n_read)
{
    assert(txt && n_read);

    while(isalnum(txt[*n_read]) || txt[*n_read] == '_')
        (*n_read)++;
}
/*
#define DEF_KEY(HASH, NAME, STD_NAME, MANGLE)               \
    case(HASH):                                             \
        tmp.type = TYPE_KEYWORD;                            \
        tmp.val.key = TOK_##MANGLE;                         \
        pos += n_read;                                      \
        PASS$(!add_token_(&tmp), return LEXER_BAD_ALLOC; ); \
        continue;                                           \

#define DEF_OP(HASH, NAME, STD_NAME, MANGLE)                \
    case(HASH):                                             \
        tmp.type = TYPE_OP;                                 \
        tmp.val.op = TOK_##MANGLE;                          \
        pos += n_read;                                      \
        PASS$(!add_token_(&tmp), return LEXER_BAD_ALLOC; ); \
        continue;                                           \

#define DEF_EMB(HASH, NAME, STD_NAME, MANGLE)               \
    case(HASH):                                             \
        tmp.type = TYPE_EMBED;                              \
        tmp.val.emb = TOK_##MANGLE;                         \
        pos += n_read;                                      \
        PASS$(!add_token_(&tmp), return LEXER_BAD_ALLOC; ); \
        continue;                                           \
*/

#define DEF_KEY(HASH, NAME, STD_NAME, MANGLE)                   \
    if(n_read == 0 || n_read == sizeof(NAME) - 1)               \
    {                                                           \
        if(strncmp(data + pos, (NAME), sizeof(NAME) - 1) == 0)  \
        {                                                       \
            printf("%s\n", NAME);                               \
            tmp.type = TYPE_KEYWORD;                            \
            tmp.val.key = TOK_##MANGLE;                         \
            pos += sizeof(NAME) - 1;                            \
            PASS$(!add_token_(&tmp), return LEXER_BAD_ALLOC; ); \
            continue;                                           \
        }                                                       \
    }                                                           \

#define DEF_OP(HASH, NAME, STD_NAME, MANGLE)                    \
    if(n_read == 0 || n_read == sizeof(NAME) - 1)               \
    {                                                           \
        if(strncmp(data + pos, (NAME), sizeof(NAME) - 1) == 0)  \
        {                                                       \
            printf("%s\n", NAME);                               \
            tmp.type = TYPE_OP;                                 \
            tmp.val.op = TOK_##MANGLE;                          \
            pos += sizeof(NAME) - 1;                            \
            PASS$(!add_token_(&tmp), return LEXER_BAD_ALLOC; ); \
            continue;                                           \
        }                                                       \
    }                                                           \

#define DEF_EMB(HASH, NAME, STD_NAME, MANGLE)                   \
    if(n_read == 0 || n_read == sizeof(NAME) - 1)               \
    {                                                           \
        if(strncmp(data + pos, (NAME), sizeof(NAME) - 1) == 0)  \
        {                                                       \
            printf("%s\n", NAME);                               \
            tmp.type = TYPE_EMBED;                              \
            tmp.val.emb = TOK_##MANGLE;                         \
            pos += sizeof(NAME) - 1;                            \
            PASS$(!add_token_(&tmp), return LEXER_BAD_ALLOC; ); \
            continue;                                           \
        }                                                       \
    }                                                           \

lexer_err lexer_init(char* data, ptrdiff_t data_sz)
{
    assert(data);
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
            PASS$(!add_token_(&tmp), return LEXER_BAD_ALLOC; );

            continue;
        }
        wordlen_(data + pos, &n_read);

        //ASSERT_RET$(n_read > 0, LEXER_UNKNWN_SYMB);

        //uint64_t hash = fnv1_64(data + pos, n_read);
        //switch(hash)
        //{
        //    #include "../reserved_operators.inc"
        //    #include "../reserved_keywords.inc"
        //    #include "../reserved_embedded.inc"
//
        //    default:
        //    { /*fallthrough*/ }
        //}

        #include "../reserved_operators.inc"
        #include "../reserved_keywords.inc"
        #include "../reserved_embedded.inc"
        /* else */
           { void(0); }

        if(!n_read)
        {
            printf("Unknown symbols in input\n");
            exit(1);
        }

        tmp.type       = TYPE_ID;
        tmp.val.name   = data + pos;
        tmp.name_len   = n_read;

        PASS$(!add_token_(&tmp), return LEXER_BAD_ALLOC; );
        printf("%s\n", data + pos);
        pos += n_read;
    }

    TOKEN_ARRAY_.eof = TOKEN_ARRAY_.size;
    tmp.type = TYPE_EOF;
    PASS$(!add_token_(&tmp), return LEXER_BAD_ALLOC; );

    return LEXER_NOERR;
}

lexer_err lexer_dstr()
{
    printf("destructor");
    return LEXER_NOERR;
}

#undef DEF_OP
#undef DEF_KEY
#undef DEF_EMB

void consume(Token* tok)
{
    assert(tok);
    assert(TOKEN_ARRAY_.pos <= TOKEN_ARRAY_.eof);

    *tok = TOKEN_ARRAY_.data[TOKEN_ARRAY_.pos];
    TOKEN_ARRAY_.pos++;
}

void peek(Token* tok, ptrdiff_t offset)
{
    assert(tok);

    if(TOKEN_ARRAY_.pos + offset < 0 || TOKEN_ARRAY_.pos + offset > TOKEN_ARRAY_.eof)
    {
        *tok = TOKEN_ARRAY_.data[TOKEN_ARRAY_.eof];
        
        return;
    }

    *tok = TOKEN_ARRAY_.data[TOKEN_ARRAY_.pos + offset];
}

#define DEF_OP(HASH, NAME, STD_NAME, MANGLE)    \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (STD_NAME));      \
        break;                                  \

#define DEF_KEY(HASH, NAME, STD_NAME, MANGLE)   \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (STD_NAME));      \
        break;                                  \
    
#define DEF_EMB(HASH, NAME, STD_NAME, MANGLE)   \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (STD_NAME));      \
        break;                                  \

#define DEF_AUX(NAME, MANGLE)                   \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (NAME));          \
        break;                                  \

char* demangle(const Token* tok)
{
    assert(tok);
    
    static char buffer[1000] = "";

    switch(tok->type)
    {
        case TYPE_NUMBER:
            sprintf(buffer, "%lg", tok->val.num);
            break;

        case TYPE_VAR : case TYPE_FUNC : case TYPE_CONSTANT :
            for(int iter = 0; iter < tok->name_len; iter++)
                buffer[iter] = tok->val.name[iter];

            buffer[tok->name_len] = '\0';
            break;

        case TYPE_OP:
        {
            switch(tok->val.op)
            {
                #include "../reserved_operators.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_KEYWORD:
        {
            switch(tok->val.key)
            {
                #include "../reserved_keywords.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_EMBED:
        {
            switch(tok->val.emb)
            {
                #include "../reserved_embedded.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_AUX:
        {
            switch(tok->val.aux)
            {
                #include "../reserved_auxiliary.inc"

                default:
                    assert(0);
            }
            
            break;
        }
        default: case TYPE_NOTYPE : case TYPE_EOF : case TYPE_ID :
            ASSERT$(0, LEXER_NOTFOUND, assert(0); );
            break;
    }

    return buffer;
}

#undef DEF_OP
#undef DEF_KEY
#undef DEF_EMB
